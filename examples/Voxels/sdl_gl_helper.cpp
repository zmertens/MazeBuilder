#include "sdl_gl_helper.h"

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

#include "player.h"
#include "scene_node.h"
#include "shader.h"

bool sdl_gl_helper::initialize(std::string_view title, int width, int height) noexcept
{
    auto init_func = [this, title, width, height]()
    {
        if (!SDL_SetAppMetadata("Maze builder with voxels", title.data(), "voxels;maze;c++;sdl"))
        {
            return false;
        }

        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, title.data());
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "maze builder");
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, "MIT License");
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "simulation;game;voxel");
        SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, title.data());

#if defined(MAZE_DEBUG)
        SDL_Log("OpenGL Setting :: SDL_GL_CONTEXT_DEBUG_FLAG\n");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
#endif

#if defined(__EMSCRIPTEN__)
        SDL_Log("OpenGL Setting :: SDL_GL_CONTEXT_PROFILE_ES\n");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
        SDL_Log("OpenGL Setting :: SDL_GL_CONTEXT_PROFILE_CORE\n");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        constexpr auto window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE |
            SDL_WINDOW_INPUT_FOCUS;

        this->window = SDL_CreateWindow(title.data(), width, height, window_flags);

        if (!this->window)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateWindow failed (%s)\n", SDL_GetError());
            return false;
        }

        this->gl_context = SDL_GL_CreateContext(this->window);

        if (!this->gl_context)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_GL_CreateContext failed (%s)\n", SDL_GetError());
            return false;
        }

        SDL_GL_MakeCurrent(this->window, this->gl_context);

        SDL_GL_SetSwapInterval(1);

        SDL_ShowWindow(window);
        SDL_SetWindowRelativeMouseMode(window, false);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

#if !defined(__EMSCRIPTEN__)
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL loader failed (%s)\n", SDL_GetError());
            SDL_Quit();
            return false;
        }
#endif

        print_opengl_info();

        return true;
    };

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::call_once(m_initialized_flag, init_func);
        return true;
    }
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_Init failed: %s\n", SDL_GetError());
    return false;
}

void sdl_gl_helper::destroy_and_quit() noexcept
{
    // Prevent double-destruction
    if (!this->window && !this->gl_context)
    {
#if defined(MAZE_DEBUG)
        SDL_Log("sdl_helper::destroy_and_quit() - Already destroyed, skipping\n");
#endif

        return;
    }

    if (this->gl_context)
    {
#if defined(MAZE_DEBUG)
        SDL_Log("sdl_helper::destroy_and_quit() - Destroying gl_context %p\n", static_cast<void*>(gl_context));
#endif

        // SDL_Des (this->gl_context);
        this->gl_context = nullptr;
    }

    if (window)
    {
#if defined(MAZE_DEBUG)
        SDL_Log("sdl_helper::destroy_and_quit() - Destroying window %p\n", static_cast<void*>(window));
#endif

        SDL_DestroyWindow(window);
        window = nullptr;
    }

    // Only call SDL_Quit() if we actually destroyed something
    if (SDL_WasInit(0) != 0)
    {
#if defined(MAZE_DEBUG)
        SDL_Log("sdl_helper::destroy_and_quit() - Calling SDL_Quit()\n");
#endif

        SDL_Quit();
    }
}

std::int32_t sdl_gl_helper::get_scale_factor() const noexcept
{
    int window_width, window_height;
    int buffer_width, buffer_height;
    SDL_GetWindowSize(this->window, &window_width, &window_height);
    SDL_GetWindowSizeInPixels(this->window, &buffer_width, &buffer_height);
    return buffer_width / window_width;
}

std::pair<std::int32_t, std::int32_t> sdl_gl_helper::get_window_size() const noexcept
{
    auto w = 0, h = 0;
    SDL_GetWindowSize(this->window, &w, &h);
    return {w, h};
}

std::pair<std::int32_t, std::int32_t> sdl_gl_helper::get_window_size_in_pixels() const noexcept
{
    auto w = 0, h = 0;
    SDL_GetWindowSizeInPixels(this->window, &w, &h);
    return {w, h};
}

void sdl_gl_helper::print_display_modes() noexcept
{
    const SDL_DisplayID display = SDL_GetPrimaryDisplay();
    int num_modes = 0;
    if (const SDL_DisplayMode* const * modes = SDL_GetFullscreenDisplayModes(display, &num_modes))
    {
        for (int i = 0; i < num_modes; ++i)
        {
            const SDL_DisplayMode* mode = modes[i];
            SDL_Log("Display %" SDL_PRIu32 " mode %d: %dx%d@%gx %gHz\n",
                    display, i, mode->w, mode->h, mode->pixel_density, mode->refresh_rate);
        }
    }
}

void sdl_gl_helper::print_opengl_info() noexcept
{
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    SDL_Log("-------------------------------------------------------------\n");
    SDL_Log("GL Vendor    : %s\n", vendor);
    SDL_Log("GL Renderer  : %s\n", renderer);
    SDL_Log("GL Version   : %s\n", version);
    SDL_Log("GL Version   : %d.%d\n", major, minor);
    SDL_Log("GLSL Version : %s\n", glslVersion);
    SDL_Log("-------------------------------------------------------------\n");
}

void sdl_gl_helper::set_window_icon(std::string_view icon_path) noexcept
{
    SDL_Surface* icon_surface = SDL_LoadBMP_IO(SDL_IOFromFile(icon_path.data(), "rb"), true);
    if (icon_surface)
    {
        SDL_SetWindowIcon(this->window, icon_surface);
        SDL_DestroySurface(icon_surface);
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "(SDL) Couldn't load icon at %s\n", icon_path.data());
    }
}

void sdl_gl_helper::del_buffer(const std::uint32_t buffer) noexcept
{
    glDeleteBuffers(1, &buffer);
}

std::uint32_t sdl_gl_helper::gen_buffer(const std::size_t size, const float* data) noexcept
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(size), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return buffer;
}

float* sdl_gl_helper::malloc_faces(const std::size_t components, const std::size_t faces) noexcept
{
    return static_cast<GLfloat*>(SDL_malloc(sizeof(GLfloat) * 6 * components * faces));
}

std::uint32_t sdl_gl_helper::gen_faces(const std::size_t components, const std::size_t faces,
                                       const float* data) noexcept
{
    const GLuint buffer = gen_buffer(sizeof(GLfloat) * 6 * components * faces, data);
    return buffer;
}

void sdl_gl_helper::draw_triangles_3d_ao(const attrib* a, const std::uint32_t buffer, const int count) noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(a->position);
    glEnableVertexAttribArray(a->normal);
    glEnableVertexAttribArray(a->uv);
    glVertexAttribPointer(a->position, 3, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 10, nullptr);
    glVertexAttribPointer(a->normal, 3, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 10, reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3));
    glVertexAttribPointer(a->uv, 4, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 10, reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 6));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(a->position);
    glDisableVertexAttribArray(a->normal);
    glDisableVertexAttribArray(a->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sdl_gl_helper::draw_triangles_3d_text(const attrib* a, const std::uint32_t buffer, const int count) noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(a->position);
    glEnableVertexAttribArray(a->uv);
    glVertexAttribPointer(a->position, 3, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 5, nullptr);
    glVertexAttribPointer(a->uv, 2, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 5, reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(a->position);
    glDisableVertexAttribArray(a->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sdl_gl_helper::draw_triangles_3d(const attrib* a, const std::uint32_t buffer, const int count) const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    glEnableVertexAttribArray(a->position);

    glEnableVertexAttribArray(a->normal);
    glEnableVertexAttribArray(a->uv);

    glVertexAttribPointer(a->position, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, nullptr);
    glVertexAttribPointer(a->normal, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8,
                          reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3));
    glVertexAttribPointer(a->uv, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8,
                          reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 6));

    glDrawArrays(GL_TRIANGLES, 0, count);

    glDisableVertexAttribArray(a->position);
    glDisableVertexAttribArray(a->normal);
    glDisableVertexAttribArray(a->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sdl_gl_helper::draw_triangles_2d(const attrib* a, const std::uint32_t buffer,
                                      const std::size_t count) const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(a->position);
    glEnableVertexAttribArray(a->uv);
    glVertexAttribPointer(a->position, 2, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 4, 0);
    glVertexAttribPointer(a->uv, 2, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 4, reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 2));
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(count));
    glDisableVertexAttribArray(a->position);
    glDisableVertexAttribArray(a->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sdl_gl_helper::draw_lines(const attrib* a, const std::uint32_t buffer, const int components,
                               const int count) const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(a->position);
    glVertexAttribPointer(
        a->position, components, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_LINES, 0, count);
    glDisableVertexAttribArray(a->position);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sdl_gl_helper::draw_chunk(const attrib* a, const scene_node* chunk) const noexcept
{
    draw_triangles_3d_ao(a, chunk->buffer, chunk->faces * 6);
}

void sdl_gl_helper::draw_item(const attrib* a, const GLuint buffer, const int count) const noexcept
{
    draw_triangles_3d_ao(a, buffer, count);
}

void sdl_gl_helper::draw_text(const attrib* a, const std::uint32_t buffer, const std::size_t length) const noexcept
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_triangles_2d(a, buffer, length * 6);
    glDisable(GL_BLEND);
}

void sdl_gl_helper::draw_signs(const attrib* a, const scene_node* chunk) const noexcept
{
    glDisable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-8, -1024);
    draw_triangles_3d_text(a, chunk->sign_buffer, chunk->sign_faces * 6);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
}

void sdl_gl_helper::draw_sign(const attrib* a, const std::uint32_t buffer, const int length) const noexcept
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-8, -1024);
    draw_triangles_3d_text(a, buffer, length * 6);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void sdl_gl_helper::draw_cube(const attrib* a, const std::uint32_t buffer) const noexcept
{
    draw_item(a, buffer, 36);
}

void sdl_gl_helper::draw_plant(const attrib* a, const std::uint32_t buffer) const noexcept
{
    draw_item(a, buffer, 24);
}

void sdl_gl_helper::draw_player(const attrib* a, const player* _player) const noexcept
{
    draw_cube(a, _player->get_buffer());
}
