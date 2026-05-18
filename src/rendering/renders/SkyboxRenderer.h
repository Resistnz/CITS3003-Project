#ifndef SKYBOX_RENDERER_H
#define SKYBOX_RENDERER_H

#include <glad/gl.h>
#include <glm/glm.hpp>

#include "rendering/renders/shaders/ShaderInterface.h"

// A renderer for drawing a cubemap skybox behind all the scene
// Adapted from https://learnopengl.com/Advanced-OpenGL/Cubemaps
class SkyboxRenderer {
    class SkyboxShader : public ShaderInterface {
        int view_matrix_location{};
        int projection_matrix_location{};
        int inverse_gamma_location{};
    public:
        SkyboxShader();

        void set_matrices(const glm::mat4& view, const glm::mat4& projection);
        void set_inverse_gamma(float inverse_gamma);
    private:
        void get_uniforms_set_bindings();
    };

    SkyboxShader shader;
    uint vao{};
    uint vbo{};
    uint cubemap_texture{};
    bool has_cubemap = false;

public:
    SkyboxRenderer();
    ~SkyboxRenderer();

    /// Set the cubemap texture to render
    void set_cubemap(uint texture_id);
    [[nodiscard]] bool is_loaded() const;
    void render(const glm::mat4& view_matrix, const glm::mat4& projection_matrix, float gamma);
    bool refresh_shaders();
};

#endif
