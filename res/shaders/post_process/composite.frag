#version 410 core
out vec4 FragColor;
in vec2 TexCoords;

// Toggles
uniform sampler2D sceneTexture;
uniform bool enableVignette;
uniform bool enableColorGrading;
uniform bool enableFilmGrain;

// Vignette
uniform float vignetteStrength;
uniform float vignetteExtent;

// Color Grading
uniform float exposure;
uniform float contrast;
uniform float saturation;

// Film Grain
uniform float filmGrainStrength;
uniform float time;

// Random noise function
// https://thebookofshaders.com/11/
float noise(vec2 uv) {
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}


void main() {
    vec3 color = texture(sceneTexture, TexCoords).rgb;
    
    // Colour Grading
    if (enableColorGrading) {
        // Exposure mapping
        color = vec3(1.0) - exp(-color * exposure);
        
        // Contrast
        color = (color - 0.5) * max(contrast, 0.0) + 0.5;
        
        // Saturation
        float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
        color = mix(vec3(luminance), color, saturation);
    }
    
    // Vignette
    if (enableVignette) {
        vec2 dist = TexCoords - 0.5;
        // Adjust for aspect ratio roughly if we want, but simple distance works too
        float vFactor = smoothstep(vignetteExtent, vignetteExtent - vignetteStrength, length(dist));
        color *= vFactor;
    }

    // Film Grain
    if (enableFilmGrain) {
        // Add time to texture coordinates to animate the noise
        float n = noise(TexCoords + vec2(time, -time));
        // Map noise from [0, 1] to [-1, 1]
        n = n * 2.0 - 1.0;
        color += color * n * filmGrainStrength;
    }
    
    FragColor = vec4(color, 1.0);
}
