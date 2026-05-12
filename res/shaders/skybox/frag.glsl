#version 410 core

in vec3 TexCoords;

layout(location = 0) out vec4 FragColor;

uniform samplerCube skybox;
uniform float inverse_gamma;

void main() {
    FragColor = texture(skybox, TexCoords);
    FragColor.rgb = pow(FragColor.rgb, vec3(inverse_gamma)); // Gamma correction
}
