#ifndef BLOOM_EFFECTS_H
#define BLOOM_EFFECTS_H

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include "gl_resource_manager.h"
#include <memory>
#include <vector>

namespace craft_rendering {

/// @brief Modern C++ bloom effects implementation with RAII resource management
/// @details Implements HDR rendering with two-pass Gaussian blur for bloom effect
class BloomEffects {
public:
    BloomEffects();
    ~BloomEffects() = default;

    // Non-copyable but movable
    BloomEffects(const BloomEffects&) = delete;
    BloomEffects& operator=(const BloomEffects&) = delete;
    BloomEffects(BloomEffects&&) = default;
    BloomEffects& operator=(BloomEffects&&) = default;

    /// @brief Generate all framebuffer objects for bloom pipeline
    /// @param width Framebuffer width
    /// @param height Framebuffer height
    void generate_framebuffers(int width, int height);

    /// @brief Reset all resources
    void reset();

    /// @brief Begin HDR rendering pass
    void begin_hdr_pass();

    /// @brief Process bloom with ping-pong blur
    /// @param quad_vao Fullscreen quad VAO
    /// @param blur_program Blur shader program
    /// @param blur_iterations Number of ping-pong iterations
    void process_bloom(GLuint quad_vao, GLuint blur_program, int blur_iterations = 10);

    /// @brief Finalize to texture with tone mapping
    /// @param quad_vao Fullscreen quad VAO
    /// @param screen_program Screen shader program
    /// @param apply_bloom Whether to apply bloom effect
    /// @param exposure HDR exposure value
    void finalize_to_texture(GLuint quad_vao, GLuint screen_program, bool apply_bloom, float exposure);

    /// @brief Get final composited texture
    GLuint get_final_texture() const;

    /// @brief Get HDR framebuffer for rendering
    GLuint get_hdr_framebuffer() const;

    /// @brief Check if framebuffers are initialized
    bool is_initialized() const { return m_initialized; }

private:
    // HDR framebuffer with multiple render targets
    gl::GlFramebuffer m_fbo_hdr;
    gl::GlTexture m_color_buffer_scene;      // RGB scene colors
    gl::GlTexture m_color_buffer_brightness; // Bright areas for bloom
    gl::GlRenderbuffer m_rbo_depth;

    // Ping-pong framebuffers for blur
    gl::GlFramebuffer m_fbo_pingpong[2];
    gl::GlTexture m_color_buffers_pingpong[2];

    // Final composite framebuffer
    gl::GlFramebuffer m_fbo_final;
    gl::GlTexture m_color_final;

    int m_width{0};
    int m_height{0};
    bool m_initialized{false};

    void check_framebuffer_status() const;
};

} // namespace craft_rendering

#endif // BLOOM_EFFECTS_H
