#version 410 core
#include "../common/lights.glsl"

// Per vertex data
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinate;
layout(location = 3) in vec3 tangent;

out VertexOut {
    vec2 texture_coordinate;
    vec3 ws_position;
    vec3 ws_normal;
    mat3 TBN; // we calculate the tangent matrix here
} vertex_out;

// Per instance data
uniform mat4 model_matrix;
uniform mat3 normal_matrix;





// Global data
uniform mat4 projection_view_matrix;
//uniform vec3 ws_view_position;


void main() {
    // Transform vertices
    vertex_out.ws_position = (model_matrix * vec4(vertex_position, 1.0f)).xyz;
    vertex_out.ws_normal = normalize(normal_matrix * normal);
    vertex_out.texture_coordinate = texture_coordinate;

    // Build TBN matrix
    vec3 T = normalize(normal_matrix * tangent);
    vec3 N = vertex_out.ws_normal;
    T = normalize(T - dot(T, N) * N);           // Gram-Schmidt process 
    vec3 B = cross(N, T);                       // Cross product gets the third basis vector
    vertex_out.TBN = mat3(T, B, N);

    gl_Position = projection_view_matrix * vec4(vertex_out.ws_position, 1.0f);
}
