#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <functional>

namespace craft_rendering {

/// @brief Encapsulates OpenGL state for a specific render pass
class RenderPass {
public:
    RenderPass() = default;
    ~RenderPass() = default;

    /// @brief Configure viewport
    RenderPass& viewport(int x, int y, int width, int height);

    /// @brief Configure scissor test
    RenderPass& scissor(bool enable, int x = 0, int y = 0, int width = 0, int height = 0);

    /// @brief Configure depth testing
    RenderPass& depth_test(bool enable, GLenum func = GL_LESS);

    /// @brief Configure face culling
    RenderPass& face_culling(bool enable, GLenum mode = GL_BACK, GLenum front_face = GL_CCW);

    /// @brief Configure blending
    RenderPass& blending(bool enable, GLenum src_factor = GL_SRC_ALPHA,
                        GLenum dst_factor = GL_ONE_MINUS_SRC_ALPHA);

    /// @brief Configure stencil testing
    RenderPass& stencil_test(bool enable, GLenum func = GL_ALWAYS, GLint ref = 0,
                            GLuint mask = 0xFF);

    /// @brief Configure stencil operations
    RenderPass& stencil_op(GLenum sfail = GL_KEEP, GLenum dpfail = GL_KEEP,
                          GLenum dppass = GL_KEEP);

    /// @brief Configure clear color
    RenderPass& clear_color(float r, float g, float b, float a = 1.0f);

    /// @brief Configure polygon mode (wireframe, fill, etc.)
    RenderPass& polygon_mode(GLenum face, GLenum mode);

    /// @brief Execute the render pass with configured state
    /// @param render_func Function to execute the actual rendering
    void execute(const std::function<void()>& render_func);

private:
    struct State {
        bool viewport_set = false;
        int viewport_x, viewport_y, viewport_w, viewport_h;

        bool scissor_set = false;
        bool scissor_enable = false;
        int scissor_x, scissor_y, scissor_w, scissor_h;

        bool depth_test_set = false;
        bool depth_test_enable = false;
        GLenum depth_func;

        bool culling_set = false;
        bool culling_enable = false;
        GLenum cull_mode;
        GLenum front_face_mode;

        bool blending_set = false;
        bool blending_enable = false;
        GLenum blend_src_factor;
        GLenum blend_dst_factor;

        bool stencil_test_set = false;
        bool stencil_test_enable = false;
        GLenum stencil_func;
        GLint stencil_ref;
        GLuint stencil_mask;

        bool stencil_op_set = false;
        GLenum stencil_sfail;
        GLenum stencil_dpfail;
        GLenum stencil_dppass;

        bool clear_color_set = false;
        float clear_r, clear_g, clear_b, clear_a;

        bool polygon_mode_set = false;
        GLenum polygon_face;
        GLenum polygon_draw_mode;
    } m_state;

    void apply_state();
    void restore_state();

    // Saved state for restoration
    State m_saved_state;
};

/// @brief Predefined render pass configurations
namespace RenderPasses {

/// @brief Geometry pass for 3D rendering
RenderPass geometry_pass(int width, int height);

/// @brief Post-processing pass (no depth, just fullscreen quad)
RenderPass post_process_pass(int width, int height);

/// @brief Wireframe overlay pass
RenderPass wireframe_pass(int width, int height);

/// @brief UI rendering pass
RenderPass ui_pass(int width, int height);

/// @brief Stencil outline pass
RenderPass stencil_outline_pass(int width, int height);

} // namespace RenderPasses

} // namespace craft_rendering

#endif // RENDER_PIPELINE_H

