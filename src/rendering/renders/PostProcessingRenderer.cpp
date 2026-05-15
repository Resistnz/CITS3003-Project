#include "PostProcessingRenderer.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

// --- Shaders ---

PostProcessingRenderer::CompositeShader::CompositeShader() 
    : ShaderInterface("Post-Process Composite", "post_process/vert.glsl", "post_process/composite.frag", [&](){ get_uniforms_set_bindings(); }) {
    get_uniforms_set_bindings();
}

void PostProcessingRenderer::CompositeShader::get_uniforms_set_bindings() {
    enable_vignette_loc = get_uniform_location("enableVignette");
    enable_color_grading_loc = get_uniform_location("enableColorGrading");
    enable_film_grain_loc = get_uniform_location("enableFilmGrain");

    vignette_strength_loc = get_uniform_location("vignetteStrength");
    vignette_extent_loc = get_uniform_location("vignetteExtent");

    exposure_loc = get_uniform_location("exposure");
    contrast_loc = get_uniform_location("contrast");
    saturation_loc = get_uniform_location("saturation");

    film_grain_strength_loc = get_uniform_location("filmGrainStrength");
    time_loc = get_uniform_location("time");

    set_binding("sceneTexture", 0);
}

void PostProcessingRenderer::CompositeShader::set_uniforms(const PostProcessSettings& settings, float time) {
    glProgramUniform1i(id(), enable_vignette_loc, settings.enable_vignette);
    glProgramUniform1i(id(), enable_color_grading_loc, settings.enable_color_grading);
    glProgramUniform1i(id(), enable_film_grain_loc, settings.enable_film_grain);

    glProgramUniform1f(id(), vignette_strength_loc, settings.vignette_strength);
    glProgramUniform1f(id(), vignette_extent_loc, settings.vignette_extent);

    glProgramUniform1f(id(), exposure_loc, settings.exposure);
    glProgramUniform1f(id(), contrast_loc, settings.contrast);
    glProgramUniform1f(id(), saturation_loc, settings.saturation);

    glProgramUniform1f(id(), film_grain_strength_loc, settings.film_grain_strength);
    glProgramUniform1f(id(), time_loc, time);
}

// --- Renderer ---

PostProcessingRenderer::PostProcessingRenderer() {
    setup_quad();
}

PostProcessingRenderer::~PostProcessingRenderer() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    
    if (main_fbo) {
        glDeleteFramebuffers(1, &main_fbo);
        glDeleteTextures(1, &main_color_tex);
        glDeleteRenderbuffers(1, &main_depth_rbo);
    }
}

void PostProcessingRenderer::setup_quad() {
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}

void PostProcessingRenderer::resize_fbos(uint width, uint height) {
    if (main_fbo) {
        glDeleteFramebuffers(1, &main_fbo);
        glDeleteTextures(1, &main_color_tex);
        glDeleteRenderbuffers(1, &main_depth_rbo);
    }

    current_width = width;
    current_height = height;

    // 1. Main FBO
    glGenFramebuffers(1, &main_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, main_fbo);

    glGenTextures(1, &main_color_tex);
    glBindTexture(GL_TEXTURE_2D, main_color_tex);
    // Use RGBA16F for HDR (Keeps high dynamic range values for better color grading)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, main_color_tex, 0);

    // Depth/Stencil renderbuffer (Since we no longer need to sample depth as a texture for DoF)
    glGenRenderbuffers(1, &main_depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, main_depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, main_depth_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Main FBO not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessingRenderer::update_size(const Window& window) {
    uint width = window.get_framebuffer_width();
    uint height = window.get_framebuffer_height();
    if (width != current_width || height != current_height) {
        resize_fbos(width, height);
    }
}

void PostProcessingRenderer::bind_fbo() {
    glBindFramebuffer(GL_FRAMEBUFFER, main_fbo);
}

void PostProcessingRenderer::render(float current_time) {
    // Disable depth testing for quad rendering
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(vao);

    // Pass: Composite to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT); // Clear screen

    composite_shader.use();
    composite_shader.set_uniforms(settings, current_time);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, main_color_tex);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Restore state
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

bool PostProcessingRenderer::refresh_shaders() {
    bool success = true;
    success &= composite_shader.reload_files();
    return success;
}
