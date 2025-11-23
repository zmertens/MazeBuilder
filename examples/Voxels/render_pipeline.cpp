#include "render_pipeline.h"

namespace craft_rendering {

// ============================================================================
// RenderPass Implementation
// ============================================================================

RenderPass& RenderPass::viewport(int x, int y, int width, int height) {
    m_state.viewport_set = true;
    m_state.viewport_x = x;
    m_state.viewport_y = y;
    m_state.viewport_w = width;
    m_state.viewport_h = height;
    return *this;
}

RenderPass& RenderPass::scissor(bool enable, int x, int y, int width, int height) {
    m_state.scissor_set = true;
    m_state.scissor_enable = enable;
    m_state.scissor_x = x;
    m_state.scissor_y = y;
    m_state.scissor_w = width;
    m_state.scissor_h = height;
    return *this;
}

RenderPass& RenderPass::depth_test(bool enable, GLenum func) {
    m_state.depth_test_set = true;
    m_state.depth_test_enable = enable;
    m_state.depth_func = func;
    return *this;
}

RenderPass& RenderPass::face_culling(bool enable, GLenum mode, GLenum front_face) {
    m_state.culling_set = true;
    m_state.culling_enable = enable;
    m_state.cull_mode = mode;
    m_state.front_face_mode = front_face;
    return *this;
}

RenderPass& RenderPass::blending(bool enable, GLenum src_factor, GLenum dst_factor) {
    m_state.blending_set = true;
    m_state.blending_enable = enable;
    m_state.blend_src_factor = src_factor;
    m_state.blend_dst_factor = dst_factor;
    return *this;
}

RenderPass& RenderPass::stencil_test(bool enable, GLenum func, GLint ref, GLuint mask) {
    m_state.stencil_test_set = true;
    m_state.stencil_test_enable = enable;
    m_state.stencil_func = func;
    m_state.stencil_ref = ref;
    m_state.stencil_mask = mask;
    return *this;
}

RenderPass& RenderPass::stencil_op(GLenum sfail, GLenum dpfail, GLenum dppass) {
    m_state.stencil_op_set = true;
    m_state.stencil_sfail = sfail;
    m_state.stencil_dpfail = dpfail;
    m_state.stencil_dppass = dppass;
    return *this;
}

RenderPass& RenderPass::clear_color(float r, float g, float b, float a) {
    m_state.clear_color_set = true;
    m_state.clear_r = r;
    m_state.clear_g = g;
    m_state.clear_b = b;
    m_state.clear_a = a;
    return *this;
}

RenderPass& RenderPass::polygon_mode(GLenum face, GLenum mode) {
    m_state.polygon_mode_set = true;
    m_state.polygon_face = face;
    m_state.polygon_draw_mode = mode;
    return *this;
}

void RenderPass::execute(const std::function<void()>& render_func) {
    apply_state();
    render_func();
    restore_state();
}

void RenderPass::apply_state() {
    // Apply viewport
    if (m_state.viewport_set) {
        glViewport(m_state.viewport_x, m_state.viewport_y,
                  m_state.viewport_w, m_state.viewport_h);
    }

    // Apply scissor test
    if (m_state.scissor_set) {
        if (m_state.scissor_enable) {
            glEnable(GL_SCISSOR_TEST);
            glScissor(m_state.scissor_x, m_state.scissor_y,
                     m_state.scissor_w, m_state.scissor_h);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
    }

    // Apply depth test
    if (m_state.depth_test_set) {
        if (m_state.depth_test_enable) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(m_state.depth_func);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }

    // Apply face culling
    if (m_state.culling_set) {
        if (m_state.culling_enable) {
            glEnable(GL_CULL_FACE);
            glCullFace(m_state.cull_mode);
            glFrontFace(m_state.front_face_mode);
        } else {
            glDisable(GL_CULL_FACE);
        }
    }

    // Apply blending
    if (m_state.blending_set) {
        if (m_state.blending_enable) {
            glEnable(GL_BLEND);
            glBlendFunc(m_state.blend_src_factor, m_state.blend_dst_factor);
        } else {
            glDisable(GL_BLEND);
        }
    }

    // Apply stencil test
    if (m_state.stencil_test_set) {
        if (m_state.stencil_test_enable) {
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(m_state.stencil_func, m_state.stencil_ref, m_state.stencil_mask);
        } else {
            glDisable(GL_STENCIL_TEST);
        }
    }

    // Apply stencil operations
    if (m_state.stencil_op_set) {
        glStencilOp(m_state.stencil_sfail, m_state.stencil_dpfail, m_state.stencil_dppass);
    }

    // Apply clear color
    if (m_state.clear_color_set) {
        glClearColor(m_state.clear_r, m_state.clear_g, m_state.clear_b, m_state.clear_a);
    }

    // Apply polygon mode
#if !defined(__EMSCRIPTEN__)
    if (m_state.polygon_mode_set) {
        glPolygonMode(m_state.polygon_face, m_state.polygon_draw_mode);
    }
#endif
}

void RenderPass::restore_state() {
    // State restoration could be implemented here if needed
    // For now, we rely on the next pass setting its own state
}

// ============================================================================
// Predefined Render Pass Configurations
// ============================================================================

namespace RenderPasses {

RenderPass geometry_pass(int width, int height) {
    RenderPass pass;
    pass.viewport(0, 0, width, height)
        .clear_color(1.0f, 1.0f, 1.0f, 1.0f)
        .depth_test(true, GL_LESS)
        .face_culling(true, GL_BACK, GL_CCW)
        .blending(false);
    return pass;
}

RenderPass post_process_pass(int width, int height) {
    RenderPass pass;
    pass.viewport(0, 0, width, height)
        .depth_test(false)
        .face_culling(false)
        .blending(false);
    return pass;
}

RenderPass wireframe_pass(int width, int height) {
    RenderPass pass;
    pass.viewport(0, 0, width, height)
        .depth_test(true, GL_LEQUAL)
        .face_culling(false)
        .blending(false);
#if !defined(__EMSCRIPTEN__)
    pass.polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
#endif
    return pass;
}

RenderPass ui_pass(int width, int height) {
    RenderPass pass;
    pass.viewport(0, 0, width, height)
        .depth_test(false)
        .face_culling(false)
        .blending(true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return pass;
}

RenderPass stencil_outline_pass(int width, int height) {
    RenderPass pass;
    pass.viewport(0, 0, width, height)
        .depth_test(true, GL_LESS)
        .face_culling(true, GL_BACK, GL_CCW)
        .stencil_test(true, GL_NOTEQUAL, 1, 0xFF)
        .stencil_op(GL_KEEP, GL_KEEP, GL_REPLACE)
        .blending(false);
#if !defined(__EMSCRIPTEN__)
    pass.polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
#endif
    return pass;
}

} // namespace RenderPasses

} // namespace craft_rendering

