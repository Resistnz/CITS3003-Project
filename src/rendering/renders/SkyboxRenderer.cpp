#include "SkyboxRenderer.h"

#include <glm/gtc/type_ptr.hpp>

// Cube vertex positions for a unit cube centered at the origin
// Adapted from https://learnopengl.com/Advanced-OpenGL/Cubemaps
static const float skybox_vertices[] = {
    // Back face
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    // Front face
    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    // Left face
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,

    // Right face
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,

    // Top face
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,

    // Bottom face
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
};

SkyboxRenderer::SkyboxShader::SkyboxShader()
    : ShaderInterface("Skybox", "skybox/vert.glsl", "skybox/frag.glsl",
                       [&]() { get_uniforms_set_bindings(); }) {
    get_uniforms_set_bindings();
}

void SkyboxRenderer::SkyboxShader::get_uniforms_set_bindings() {
    view_matrix_location = get_uniform_location("view");
    projection_matrix_location = get_uniform_location("projection");
    inverse_gamma_location = get_uniform_location("inverse_gamma");
    set_binding("skybox", 0);
}

void SkyboxRenderer::SkyboxShader::set_matrices(const glm::mat4& view, const glm::mat4& projection) {
    glProgramUniformMatrix4fv(id(), view_matrix_location, 1, GL_FALSE, glm::value_ptr(view));
    glProgramUniformMatrix4fv(id(), projection_matrix_location, 1, GL_FALSE, glm::value_ptr(projection));
}

void SkyboxRenderer::SkyboxShader::set_inverse_gamma(float inverse_gamma) {
    glProgramUniform1f(id(), inverse_gamma_location, inverse_gamma);
}



SkyboxRenderer::SkyboxRenderer() : shader() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), skybox_vertices, GL_STATIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

SkyboxRenderer::~SkyboxRenderer() {
    if (vao != 0) glDeleteVertexArrays(1, &vao);
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (has_cubemap) glDeleteTextures(1, &cubemap_texture);
}

void SkyboxRenderer::set_cubemap(uint texture_id) {
    // If we previously owned a cubemap get rid of it
    if (has_cubemap) {
        glDeleteTextures(1, &cubemap_texture);
    }
    cubemap_texture = texture_id;
    has_cubemap = true;
}

bool SkyboxRenderer::is_loaded() const {
    return has_cubemap;
}

void SkyboxRenderer::render(const glm::mat4& view_matrix, const glm::mat4& projection_matrix, float gamma) {
    if (!has_cubemap) return; 

    // Change depth function so the skybox passes the depth test at depth 1
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_CULL_FACE);

    shader.use();

    // Remove translation from the view matrix so the skybox is always centred on the camera
    glm::mat4 view_no_translation = glm::mat4(glm::mat3(view_matrix));
    shader.set_matrices(view_no_translation, projection_matrix);
    shader.set_inverse_gamma(1.0f / gamma);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
}

bool SkyboxRenderer::refresh_shaders() {
    return shader.reload_files();
}
