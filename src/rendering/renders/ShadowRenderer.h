#pragma once
#include "rendering/scene/MasterRenderScene.h"
#include "rendering/renders/shaders/ShaderInterface.h"

class ShadowShader : public ShaderInterface {
    int light_space_matrix_location{};
    int model_matrix_location{};
public:
    ShadowShader() : ShaderInterface("Shadow Shader", "shadows/vert.glsl", "shadows/frag.glsl", [&]() { get_uniforms(); }) {
        get_uniforms();
    }

    void get_uniforms() {
        light_space_matrix_location = get_uniform_location("light_space_matrix");
        model_matrix_location = get_uniform_location("model");
    }

    void set_light_space_matrix(const glm::mat4& matrix) {
        glProgramUniformMatrix4fv(id(), light_space_matrix_location, 1, GL_FALSE, &matrix[0][0]);
    }
    
    void set_model_matrix(const glm::mat4& matrix) {
        glProgramUniformMatrix4fv(id(), model_matrix_location, 1, GL_FALSE, &matrix[0][0]);
    }
};

class ShadowRenderer {
    unsigned int depthMapFBO;
    unsigned int depthMap;
    const unsigned int SHADOW_WIDTH = 2048;
    const unsigned int SHADOW_HEIGHT = 2048;

    ShadowShader static_shadow_shader;

public:
    ShadowRenderer();
    ~ShadowRenderer();

    // Renders the scene from the light's perspective into the FBO
    void render_shadows(const MasterRenderScene& render_scene);

    // Allows MasterRenderer to grab the texture for the main color pass
    unsigned int get_depth_map_texture() const { return depthMap; }
};
