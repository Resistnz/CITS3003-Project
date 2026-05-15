#include "MasterRenderer.h"
#include <glad/gl.h>

#include "rendering/imgui/ImGuiManager.h"
#include "scene/SceneContext.h"

MasterRenderer::MasterRenderer() : entity_renderer(), animated_entity_renderer(), emissive_entity_renderer(), render_settings() {
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

void MasterRenderer::update(const Window& window) {
    post_processing_renderer.update_size(window);
    post_processing_renderer.bind_fbo();
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glViewport(0, 0, (int) window.get_framebuffer_width(), (int) window.get_framebuffer_height());
}

void MasterRenderer::render_scene(MasterRenderScene& render_scene, const SceneContext& scene_context, const CameraInterface& camera) {
    render_scene.animator.animate(scene_context.window_manager.get_delta_time());
    entity_renderer.render(render_scene.entity_scene, render_scene.light_scene);
    animated_entity_renderer.render(render_scene.animated_entity_scene, render_scene.light_scene);
    emissive_entity_renderer.render(render_scene.emissive_entity_scene);

    // Render skybox last — depth function GL_LEQUAL ensures it only fills background pixels
    skybox_renderer.render(camera.get_view_matrix(), camera.get_projection_matrix(), camera.get_gamma());

    // Render post-processing passes to screen
    post_processing_renderer.render((float)glfwGetTime());
}

SkyboxRenderer& MasterRenderer::get_skybox_renderer() {
    return skybox_renderer;
}

void MasterRenderer::sync() {
    if (render_settings.enable_fps_cap) {
        sync_manager.sync(render_settings.fps_cap);
    }
}

void MasterRenderer::add_imgui_options_section(WindowManager& window_manager) {
    if (ImGui::CollapsingHeader("Render Settings")) {
        if (ImGui::Checkbox("Show Wireframe", &render_settings.show_wireframe)) {
            glPolygonMode(GL_FRONT_AND_BACK, render_settings.show_wireframe ? GL_LINE : GL_FILL);
        }

        if (ImGui::Checkbox("Cull Back Faces", &render_settings.cull_back_face) ||
            ImGui::Checkbox("Cull Front Faces", &render_settings.cull_front_face)) {
            if (render_settings.cull_front_face && render_settings.cull_back_face) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT_AND_BACK);
            } else if (render_settings.cull_front_face) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
            } else if (render_settings.cull_back_face) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            } else {
                glDisable(GL_CULL_FACE);
            }
        }

        if (ImGui::Checkbox("V-Sync", &render_settings.v_sync)) {
            window_manager.set_v_sync(render_settings.v_sync);
        }

        ImGui::Checkbox("Enable FPS Cap", &render_settings.enable_fps_cap);

        if (ImGui::SliderFloat("FPS Cap", &render_settings.fps_cap, 24.0f, 240.0f)) {
            if (render_settings.fps_cap < 24.0f) {
                render_settings.fps_cap = 24.0f;
            }
        }
    }

    if (ImGui::CollapsingHeader("Post-Processing Settings")) {
        ImGui::Checkbox("Enable Color Grading", &post_processing_renderer.settings.enable_color_grading);
        if (post_processing_renderer.settings.enable_color_grading) {
            ImGui::SliderFloat("Exposure", &post_processing_renderer.settings.exposure, 0.1f, 5.0f);
            ImGui::SliderFloat("Contrast", &post_processing_renderer.settings.contrast, 0.1f, 2.0f);
            ImGui::SliderFloat("Saturation", &post_processing_renderer.settings.saturation, 0.0f, 2.0f);
        }

        ImGui::Separator();
        ImGui::Checkbox("Enable Vignette", &post_processing_renderer.settings.enable_vignette);
        if (post_processing_renderer.settings.enable_vignette) {
            ImGui::SliderFloat("Vignette Extent", &post_processing_renderer.settings.vignette_extent, 0.1f, 1.5f);
            ImGui::SliderFloat("Vignette Strength", &post_processing_renderer.settings.vignette_strength, 0.1f, 1.0f);
        }

        ImGui::Separator();
        ImGui::Checkbox("Enable Film Grain", &post_processing_renderer.settings.enable_film_grain);
        if (post_processing_renderer.settings.enable_film_grain) {
            ImGui::SliderFloat("Grain Strength", &post_processing_renderer.settings.film_grain_strength, 0.0f, 0.5f);
        }
    }

    if (ImGui::CollapsingHeader("Shader Options")) {
        static int failures = 0;
        static double last_time = -std::numeric_limits<double>::infinity();
        if (ImGui::Button("Reload Shader Files")) {
            last_time = glfwGetTime();
            failures = 0;
            failures += entity_renderer.refresh_shaders() ? 0 : 1;
            failures += animated_entity_renderer.refresh_shaders() ? 0 : 1;
            failures += emissive_entity_renderer.refresh_shaders() ? 0 : 1;
            failures += skybox_renderer.refresh_shaders() ? 0 : 1;
            failures += post_processing_renderer.refresh_shaders() ? 0 : 1;
        }
        if (glfwGetTime() - 2.0 <= last_time) {
            ImGui::SameLine();
            if (failures == 0) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
                ImGui::Text("Success!");
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.0, 0.0, 1.0f));
                ImGui::Text("[%d] Failed, see Console", failures);
            }
            ImGui::PopStyleColor();
        }
    }
}
