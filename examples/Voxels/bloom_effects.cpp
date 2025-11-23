#include "bloom_effects.h"
#include <SDL3/SDL.h>

namespace craft_rendering {

BloomEffects::BloomEffects() = default;

void BloomEffects::generate_framebuffers(int width, int height) {
    m_width = width;
    m_height = height;

    // Reset any existing resources
    reset();

    // ========================================================================
    // HDR Framebuffer with Multiple Render Targets (MRT)
    // ========================================================================

    m_fbo_hdr.bind();

    // Color attachment 0: Scene color
    m_color_buffer_scene.bind(GL_TEXTURE_2D);
#if defined(__EMSCRIPTEN__)
    m_color_buffer_scene.allocate_2d(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
#else
    m_color_buffer_scene.allocate_2d_float(width, height, GL_RGBA16F);
#endif
    m_color_buffer_scene.set_parameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_color_buffer_scene.set_parameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_color_buffer_scene.set_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_color_buffer_scene.set_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                          m_color_buffer_scene.get(), 0);

    // Color attachment 1: Bright areas for bloom
    m_color_buffer_brightness.bind(GL_TEXTURE_2D);
#if defined(__EMSCRIPTEN__)
    m_color_buffer_brightness.allocate_2d(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
#else
    m_color_buffer_brightness.allocate_2d_float(width, height, GL_RGBA16F);
#endif
    m_color_buffer_brightness.set_parameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_color_buffer_brightness.set_parameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_color_buffer_brightness.set_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_color_buffer_brightness.set_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                          m_color_buffer_brightness.get(), 0);

    // Depth renderbuffer
    m_rbo_depth.allocate_storage(GL_DEPTH_COMPONENT16, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                             m_rbo_depth.get());

    // Specify which color attachments to use for rendering
    GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    check_framebuffer_status();

    // ========================================================================
    // Ping-Pong Framebuffers for Gaussian Blur
    // ========================================================================

    for (int i = 0; i < 2; ++i) {
        m_fbo_pingpong[i].bind();

        m_color_buffers_pingpong[i].bind(GL_TEXTURE_2D);
#if defined(__EMSCRIPTEN__)
        m_color_buffers_pingpong[i].allocate_2d(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
#else
        m_color_buffers_pingpong[i].allocate_2d_float(width, height, GL_RGBA16F);
#endif
        m_color_buffers_pingpong[i].set_parameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        m_color_buffers_pingpong[i].set_parameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        m_color_buffers_pingpong[i].set_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_color_buffers_pingpong[i].set_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                              m_color_buffers_pingpong[i].get(), 0);

        check_framebuffer_status();
    }

    // ========================================================================
    // Final Composite Framebuffer
    // ========================================================================

    m_fbo_final.bind();

    m_color_final.bind(GL_TEXTURE_2D);
    m_color_final.allocate_2d(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
    m_color_final.set_parameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_color_final.set_parameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_color_final.set_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_color_final.set_parameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                          m_color_final.get(), 0);

    check_framebuffer_status();

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_initialized = true;

#if defined(MAZE_DEBUG)
    SDL_Log("BloomEffects: Framebuffers initialized (w=%d, h=%d)\n", width, height);
#endif
}

void BloomEffects::reset() {
    // RAII handles cleanup automatically
    m_fbo_hdr = gl::GlFramebuffer();
    m_fbo_pingpong[0] = gl::GlFramebuffer();
    m_fbo_pingpong[1] = gl::GlFramebuffer();
    m_fbo_final = gl::GlFramebuffer();

    m_color_buffer_scene = gl::GlTexture();
    m_color_buffer_brightness = gl::GlTexture();
    m_color_buffers_pingpong[0] = gl::GlTexture();
    m_color_buffers_pingpong[1] = gl::GlTexture();
    m_color_final = gl::GlTexture();

    m_rbo_depth = gl::GlRenderbuffer();

    m_initialized = false;
}

void BloomEffects::begin_hdr_pass() {
    if (!m_initialized) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "BloomEffects: Cannot begin HDR pass, not initialized\n");
        return;
    }

    m_fbo_hdr.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void BloomEffects::process_bloom(GLuint quad_vao, GLuint blur_program, int blur_iterations) {
    if (!m_initialized) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "BloomEffects: Cannot process bloom, not initialized\n");
        return;
    }

    // Disable depth test for post-processing
    glDisable(GL_DEPTH_TEST);

    // Use blur shader
    glUseProgram(blur_program);

    // Get uniform locations
    GLint horizontal_loc = glGetUniformLocation(blur_program, "horizontal");
    GLint image_loc = glGetUniformLocation(blur_program, "image");

    // Ping-pong blur iterations
    bool horizontal = true;
    bool first_iteration = true;

    for (int i = 0; i < blur_iterations; ++i) {
        // Bind target framebuffer
        m_fbo_pingpong[horizontal ? 0 : 1].bind();

        // Set horizontal/vertical blur uniform
        glUniform1i(horizontal_loc, horizontal);

        // Bind source texture
        glActiveTexture(GL_TEXTURE0);
        if (first_iteration) {
            // First iteration uses brightness buffer from HDR pass
            m_color_buffer_brightness.bind(GL_TEXTURE_2D);
            first_iteration = false;
        } else {
            // Subsequent iterations use previous ping-pong result
            m_color_buffers_pingpong[horizontal ? 1 : 0].bind(GL_TEXTURE_2D);
        }
        glUniform1i(image_loc, 0);

        // Render fullscreen quad
        glBindVertexArray(quad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Toggle horizontal/vertical
        horizontal = !horizontal;
    }

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BloomEffects::finalize_to_texture(GLuint quad_vao, GLuint screen_program,
                                       bool apply_bloom, float exposure) {
    if (!m_initialized) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "BloomEffects: Cannot finalize, not initialized\n");
        return;
    }

    // Bind final framebuffer
    m_fbo_final.bind();
    glClear(GL_COLOR_BUFFER_BIT);

    // Use screen shader
    glUseProgram(screen_program);

    // Get uniform locations
    GLint screen_texture_loc = glGetUniformLocation(screen_program, "screenTexture");
    GLint bloom_blur_loc = glGetUniformLocation(screen_program, "bloomBlur");
    GLint do_bloom_loc = glGetUniformLocation(screen_program, "do_bloom");
    GLint exposure_loc = glGetUniformLocation(screen_program, "exposure");

    // Bind scene texture
    glActiveTexture(GL_TEXTURE0);
    m_color_buffer_scene.bind(GL_TEXTURE_2D);
    glUniform1i(screen_texture_loc, 0);

    // Bind bloom blur texture
    glActiveTexture(GL_TEXTURE1);
    m_color_buffers_pingpong[0].bind(GL_TEXTURE_2D);
    glUniform1i(bloom_blur_loc, 1);

    // Set uniforms
    glUniform1i(do_bloom_loc, apply_bloom ? 1 : 0);
    glUniform1f(exposure_loc, exposure);

    // Render fullscreen quad
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint BloomEffects::get_final_texture() const {
    return m_color_final.get();
}

GLuint BloomEffects::get_hdr_framebuffer() const {
    return m_fbo_hdr.get();
}

void BloomEffects::check_framebuffer_status() const {
    gl::GlFramebuffer::check_status();
}

} // namespace craft_rendering

