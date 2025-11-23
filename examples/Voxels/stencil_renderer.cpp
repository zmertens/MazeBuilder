#include "stencil_renderer.h"
#include <SDL3/SDL.h>

namespace craft_rendering {

StencilRenderer::StencilRenderer() = default;

bool StencilRenderer::initialize() {
#if defined(__EMSCRIPTEN__)
    const char* vertex_path = "shaders/es/stencil_vertex.es.glsl";
    const char* fragment_path = "shaders/es/stencil_fragment.es.glsl";
#else
    const char* vertex_path = "shaders/stencil_vertex.glsl";
    const char* fragment_path = "shaders/stencil_fragment.glsl";
#endif

    if (!m_stencil_program.load_from_files(vertex_path, fragment_path)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load stencil shaders\n");
        return false;
    }

    m_initialized = true;
    SDL_Log("StencilRenderer: Initialized successfully\n");
    return true;
}

void StencilRenderer::begin_stencil_write() {
    // Enable stencil test
    glEnable(GL_STENCIL_TEST);

    // Configure stencil function: always pass stencil test
    glStencilFunc(GL_ALWAYS, 1, 0xFF);

    // Configure stencil operation: replace stencil value with ref value (1)
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // Enable writing to stencil buffer
    glStencilMask(0xFF);

    // Clear stencil buffer
    glClear(GL_STENCIL_BUFFER_BIT);
}

void StencilRenderer::end_stencil_write() {
    // Disable writing to stencil buffer
    glStencilMask(0x00);
}

void StencilRenderer::render_outline(float scale_factor) {
    // Configure stencil function: only pass where stencil != 1
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    // Disable writing to stencil buffer
    glStencilMask(0x00);

    // Disable depth test to draw outline on top
    glDisable(GL_DEPTH_TEST);

    // Use stencil shader program
    m_stencil_program.use();

    // Set scale factor uniform if needed
    GLint scale_loc = m_stencil_program.get_uniform_location("scale_factor");
    if (scale_loc != -1) {
        glUniform1f(scale_loc, scale_factor);
    }
}

void StencilRenderer::clear_stencil() {
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilMask(0xFF);
}

} // namespace craft_rendering

