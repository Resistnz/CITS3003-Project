#ifndef NUM_PL
#define NUM_PL 0
#endif

// Material Properties
struct Material {
    vec3 diffuse_tint;
    vec3 specular_tint;
    vec3 ambient_tint;
    float shininess;
};

// Light Data
struct LightCalculatioData {
    vec3 ws_frag_position;
    vec3 ws_view_dir;
    vec3 ws_normal;
};

struct PointLightData {
    vec3 position;
    vec3 colour;
    float attenuation;
};

struct DirectionalLightData {
    vec3 direction;
    vec3 colour;
    mat4 light_space_matrix; 
};

// Calculations

const float ambient_factor = 0.002f;

// Point Lights
void point_light_calculation(PointLightData point_light, LightCalculatioData calculation_data, float shininess, inout vec3 total_diffuse, inout vec3 total_specular, inout vec3 total_ambient) {
    vec3 ws_light_offset = point_light.position - calculation_data.ws_frag_position;

    float distance = length(ws_light_offset); // Distance magnitude for falloff

    // Ambient
    vec3 ambient_component = ambient_factor * point_light.colour;

    // Diffuse
    vec3 ws_light_dir = normalize(ws_light_offset);
    float diffuse_factor = max(dot(ws_light_dir, calculation_data.ws_normal), 0.0f);
    vec3 diffuse_component = diffuse_factor * point_light.colour;

    // Specular
    vec3 ws_halfway_dir = normalize(ws_light_dir + calculation_data.ws_view_dir);
    float specular_factor = pow(max(dot(calculation_data.ws_normal, ws_halfway_dir), 0.0f), shininess);
    vec3 specular_component = specular_factor * point_light.colour;
    point_light.attenuation = 30.0f * point_light.attenuation; // Scale the attenuation for better visual results, as the default values are too low

    // Calculate the falloff based on distance and attenuation, following the data and approach provided in the openGL tutorial https://learnopengl.com/Lighting/Light-casters#Attenuation
    float linear_multiplier = 4.5/point_light.attenuation; // Linear attenuation factor
    float quadratic_multiplier = 75.0/(point_light.attenuation * point_light.attenuation); // Quadratic attenuation factor 

    float falloff = 1.0f / (1.0f + linear_multiplier * distance + quadratic_multiplier * distance * distance); 
    //falloff = 1.0f / distance; // Alternative linear falloff for testing
    // Consider the falloff for each point light and accumulate
    total_diffuse += diffuse_component * falloff; 
    total_specular += specular_component * falloff;
    total_ambient += ambient_component * falloff; 
}

// Total Calculation

struct LightingResult {
    vec3 total_diffuse;
    vec3 total_specular;
    vec3 total_ambient;
};

// --- SHADOW MAPPING ---
float calculate_shadow(vec4 frag_pos_light_space, vec3 normal, vec3 light_dir, sampler2D shadow_map) {
    // 1. Perspective divide (turns it into normalized device coordinates [-1, 1])
    vec3 proj_coords = frag_pos_light_space.xyz / frag_pos_light_space.w;
    
    // 2. Transform to [0, 1] range for texture sampling
    proj_coords = proj_coords * 0.5 + 0.5;
    
    // Ignore fragments outside the light's projection frustum
    if(proj_coords.z > 1.0) return 0.0;
    
    float current_depth = proj_coords.z;
    
    // 3. Apply a small bias to prevent "Shadow Acne" (self-shadowing artifacts)
    float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);
    
    // 4. PCF (Percentage Closer Filtering) for softer shadow edges
    float shadow = 0.0;
    vec2 texel_size = 1.0 / textureSize(shadow_map, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcf_depth = texture(shadow_map, proj_coords.xy + vec2(x, y) * texel_size).r; 
            shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

// Directional Lights
void directional_light_calculation(DirectionalLightData direction_light, LightCalculatioData calculation_data, float shininess, sampler2D shadow_map, inout vec3 total_diffuse, inout vec3 total_specular, inout vec3 total_ambient) {
    // Ambient
    vec3 ambient_component = ambient_factor * direction_light.colour;

    // Diffuse
    vec3 ws_light_dir = normalize(-direction_light.direction);
    float diffuse_factor = max(dot(ws_light_dir, calculation_data.ws_normal), 0.0f);
    vec3 diffuse_component = diffuse_factor * direction_light.colour;

    // Specular
    vec3 ws_halfway_dir = normalize(ws_light_dir + calculation_data.ws_view_dir);
    float specular_factor = pow(max(dot(calculation_data.ws_normal, ws_halfway_dir), 0.0f), shininess);
    vec3 specular_component = specular_factor * direction_light.colour;

    // Calculate shadow
    vec4 frag_pos_light_space = direction_light.light_space_matrix * vec4(calculation_data.ws_frag_position, 1.0);
    float shadow = calculate_shadow(frag_pos_light_space, calculation_data.ws_normal, ws_light_dir, shadow_map);

    // Consider the falloff for each point light and accumulate
    total_diffuse += (1.0 - shadow) * diffuse_component; 
    total_specular += (1.0 - shadow) * specular_component;
    total_ambient += ambient_component; 
}

LightingResult total_light_calculation(LightCalculatioData light_calculation_data, Material material, sampler2D shadow_map
        #if NUM_PL > 0
        ,PointLightData point_lights[NUM_PL]
        #endif
        #if NUM_DL > 0
        ,DirectionalLightData direction_lights[NUM_DL]
        #endif
    ) {

    vec3 total_diffuse = vec3(0.0f);
    vec3 total_specular = vec3(0.0f);
    vec3 total_ambient = vec3(0.0f);

    #if NUM_PL > 0
    for (int i = 0; i < NUM_PL; i++) {
        point_light_calculation(point_lights[i], light_calculation_data, material.shininess, total_diffuse, total_specular, total_ambient);
    }
    #endif

    #if NUM_DL > 0
    for (int i = 0; i < NUM_DL; i++) {
        directional_light_calculation(direction_lights[i], light_calculation_data, material.shininess, shadow_map, total_diffuse, total_specular, total_ambient);
    }
    #endif

    #if NUM_PL > 0
    total_ambient /= float(NUM_PL);
    #endif



    total_diffuse *= material.diffuse_tint;
    total_specular *= material.specular_tint;
    total_ambient *= material.ambient_tint;

    return LightingResult(total_diffuse, total_specular, total_ambient);
}

vec3 resolve_textured_light_calculation(LightingResult result, sampler2D diffuse_texture, sampler2D specular_map, vec2 texture_coordinate) {
    vec3 texture_colour = texture(diffuse_texture, texture_coordinate).rgb;
    vec3 specular_map_sample = texture(specular_map, texture_coordinate).rgb;

    vec3 textured_diffuse = result.total_diffuse * texture_colour;
    vec3 sampled_specular = result.total_specular * specular_map_sample;
    vec3 textured_ambient = result.total_ambient * texture_colour;

    // Mix the diffuse and ambient so that there is no ambient in bright scenes
    return max(textured_diffuse, textured_ambient) + sampled_specular;
}