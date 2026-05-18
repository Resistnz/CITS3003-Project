#version 410 core
#include "../common/lights.glsl"

in VertexOut {
    vec2 texture_coordinate;
    vec3 ws_position;
    vec3 ws_normal;
    mat3 TBN;
} frag_in;

layout(location = 0) out vec4 out_colour;

// Global Data
uniform float inverse_gamma;

uniform sampler2D diffuse_texture;
uniform sampler2D specular_map_texture;
uniform sampler2D normal_map_texture;
uniform sampler2D depth_map_texture;

uniform float uv_scale;

uniform vec3 ws_view_position;

// Material properties
uniform vec3 diffuse_tint;
uniform vec3 specular_tint;
uniform vec3 ambient_tint;
uniform float shininess;
uniform float depth;

// Light Data
#if NUM_PL > 0
layout (std140) uniform PointLightArray {
    PointLightData point_lights[NUM_PL];
};
#endif

#if NUM_DL > 0
layout (std140) uniform DirectionalLightArray {
    DirectionalLightData direction_lights[NUM_DL];
};
#endif

// Parallax mapping function adapted from learnopengl.com, with some adjustments to the layer count and depth offset.
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    if (depth < 0.001) return texCoords;

    const float minLayers = 8.0;
    const float maxLayers = 80.0;
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    vec2 P = viewDir.xy * depth;
    vec2 deltaTexCoords = P / numLayers;
    vec2  currentTexCoords = texCoords;
    float currentDepthMapValue = 1.0 - texture(depth_map_texture, currentTexCoords).r;

    // Step through layers along the view direction until the sampled depth
    // exceeds the current layer depth, finding the intersection.
    while(currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1.0 - texture(depth_map_texture, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    // Occlusion interpolation: blend between the two layers straddling the
    // intersection point for a smoother result without hard layer transitions.
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = 1.0 - texture(depth_map_texture, prevTexCoords).r - currentLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

void main() {
    vec2 scaled_uv = frag_in.texture_coordinate * uv_scale;

    vec3 ws_view_dir = normalize(ws_view_position - frag_in.ws_position);

    vec3 ts_view_dir = normalize(transpose(frag_in.TBN) * ws_view_dir);
    vec2 parallax_uv = ParallaxMapping(scaled_uv, ts_view_dir);
    
    if(parallax_uv.x > uv_scale || parallax_uv.y > uv_scale || parallax_uv.x < 0.0 || parallax_uv.y < 0.0)
        discard;

    // Sample normal map and transform it into world space
    vec3 tangent_normal = texture(normal_map_texture, parallax_uv).rgb;
    tangent_normal = normalize(tangent_normal * 2.0 - 1.0); //
    vec3 ws_normal = normalize(frag_in.TBN * tangent_normal); // Ensure the normal is normalized, as it was was previously lost when interpolated


    LightCalculatioData light_calculation_data = LightCalculatioData(frag_in.ws_position, ws_view_dir, ws_normal);
    Material material = Material(diffuse_tint, specular_tint, ambient_tint, shininess);

    LightingResult lighting_result = total_light_calculation(light_calculation_data, material
        #if NUM_PL > 0
        ,point_lights
        #endif
        #if NUM_DL > 0
        ,direction_lights
        #endif
    );

    // Combine textures with the fragment lighting result
    vec3 resolved_lighting = resolve_textured_light_calculation(lighting_result, diffuse_texture, specular_map_texture, parallax_uv);

    out_colour = vec4(resolved_lighting, 1.0f);
    out_colour.rgb = pow(out_colour.rgb, vec3(inverse_gamma));
}
