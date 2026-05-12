#ifndef POST_PROCESSING_RENDERER_H
#define POST_PROCESSING_RENDERER_H

#include <glad/gl.h>
#include <glm/glm.hpp>
#include "rendering/renders/shaders/ShaderInterface.h"
#include "system_interfaces/Window.h"

struct PostProcessSettings {
    bool enable_vignette = true;
    bool enable_color_grading = true;
    bool enable_film_grain = true;

    // Vignette
    float vignette_strength = 0.6f;
    float vignette_extent = 1.1f;

    // Color Grading
    float exposure = 1.3f;
    float contrast = 1.2f;
    float saturation = 1.3f;

    // Film Grain
    float film_grain_strength = 0.05f;
};

class PostProcessingRenderer {
    class CompositeShader : public ShaderInterface {
        // Toggles
        int enable_vignette_loc{};
        int enable_color_grading_loc{};
        int enable_film_grain_loc{};

        // Vignette
        int vignette_strength_loc{};
        int vignette_extent_loc{};

        // Color Grading
        int exposure_loc{};
        int contrast_loc{};
        int saturation_loc{};

        // Film Grain
        int film_grain_strength_loc{};
        int time_loc{}; // For animating grain

    public:
        CompositeShader();
        void set_uniforms(const PostProcessSettings& settings, float time);
    private:
        void get_uniforms_set_bindings();
    };

    CompositeShader composite_shader;

    uint vao{};
    uint vbo{};

    uint main_fbo{};
    uint main_color_tex{};
    uint main_depth_rbo{};

    uint current_width = 0;
    uint current_height = 0;

    void setup_quad();
    void resize_fbos(uint width, uint height);

public:
    PostProcessSettings settings;

    PostProcessingRenderer();
    ~PostProcessingRenderer();

    /// Updates FBO sizes if window size changes
    void update_size(const Window& window);

    /// Binds the main FBO for scene rendering
    void bind_fbo();

    /// Renders the post processing passes to the default framebuffer
    void render(float current_time);

    bool refresh_shaders();
};

#endif // POST_PROCESSING_RENDERER_H
