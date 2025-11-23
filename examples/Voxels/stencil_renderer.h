#ifndef STENCIL_RENDERER_H
#define STENCIL_RENDERER_H

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include "gl_resource_manager.h"
#include <vector>

namespace craft_rendering {

/// @brief Renders outlined shapes using stencil buffer
class StencilRenderer {
public:
    StencilRenderer();
    ~StencilRenderer() = default;

    /// @brief Initialize stencil renderer with shaders
    bool initialize();

    /// @brief Begin stencil writing (first pass)
    void begin_stencil_write();

    /// @brief End stencil writing
    void end_stencil_write();

    /// @brief Render outline using stencil buffer (second pass)
    /// @param scale_factor Scale factor for outline thickness
    void render_outline(float scale_factor = 1.05f);

    /// @brief Clear stencil buffer
    void clear_stencil();

    /// @brief Check if initialized
    bool is_initialized() const { return m_initialized; }

    /// @brief Get stencil shader program
    GLuint get_stencil_program() const { return m_stencil_program.get(); }

private:
    gl::GlShaderProgram m_stencil_program;
    bool m_initialized{false};
};

} // namespace craft_rendering

#endif // STENCIL_RENDERER_H

