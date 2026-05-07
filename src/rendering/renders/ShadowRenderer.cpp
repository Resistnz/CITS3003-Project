#include "ShadowRenderer.h"
#include <glad/gl.h>

ShadowRenderer::ShadowRenderer() {
    // 1. Create a texture for the depth map
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    // 2. Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); // Better for shadows than REPEAT
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  
}

ShadowRenderer::~ShadowRenderer() {
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);
}

#include <glm/gtc/matrix_transform.hpp>

void ShadowRenderer::render_shadows(const MasterRenderScene& render_scene) {
    if (render_scene.light_scene.directional_lights.empty()) return;

    // Just use the first directional light for shadows
    auto dir_light = *render_scene.light_scene.directional_lights.begin();

    // 1. Calculate Light Space Matrix
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 25.0f); // Adjust boundaries as needed!
    glm::mat4 lightView = glm::lookAt(dir_light->position, dir_light->position + dir_light->direction, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    // Update the light's matrix so it goes to the GPU UBO later
    dir_light->light_space_matrix = lightSpaceMatrix;

    // 2. Bind depthMapFBO and set viewport
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    
    // 3. Clear GL_DEPTH_BUFFER_BIT
    glClear(GL_DEPTH_BUFFER_BIT);

    // 4. Render entities
    static_shadow_shader.use();
    static_shadow_shader.set_light_space_matrix(lightSpaceMatrix);

    for (const auto& entity : render_scene.entity_scene.entities) {
        static_shadow_shader.set_model_matrix(entity->instance_data.model_matrix);
        glBindVertexArray(entity->model->get_vao());
        glDrawElementsBaseVertex(GL_TRIANGLES, entity->model->get_index_count(), GL_UNSIGNED_INT, nullptr, entity->model->get_vertex_offset());
    }

    // TODO: You can add another loop here for animated_entity_scene using an AnimatedShadowShader!

    // 5. Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
