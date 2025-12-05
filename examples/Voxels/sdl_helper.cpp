#include "sdl_helper.h"

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

bool sdl_helper::initialize(std::string_view title, int width, int height) noexcept
{
    auto init_func = [this, title, width, height]()
    {
        if (!SDL_SetAppMetadata("Maze builder with voxels", title.data(), "voxels;maze;c++;sdl")) {

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

        const Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS;
        
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
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL loader failed (%s)\n", SDL_GetError());
            SDL_Quit();
            return false;
        }
#endif

        return true;
    };

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::call_once(m_initialized_flag, init_func);
        return true;
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
}

void sdl_helper::destroy_and_quit() noexcept
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

std::int32_t sdl_helper::get_scale_factor() const noexcept
{
    int window_width, window_height;
    int buffer_width, buffer_height;
    SDL_GetWindowSize(this->window, &window_width, &window_height);
    SDL_GetWindowSizeInPixels(this->window, &buffer_width, &buffer_height);
    return buffer_width / window_width;
}

void sdl_helper::print_display_modes() const noexcept
{
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    int num_modes = 0;
    const SDL_DisplayMode *const *modes = SDL_GetFullscreenDisplayModes(display, &num_modes);
    if (modes)
    {
        for (int i = 0; i < num_modes; ++i)
        {
            const SDL_DisplayMode *mode = modes[i];
            SDL_Log("Display %" SDL_PRIu32 " mode %d: %dx%d@%gx %gHz\n",
                    display, i, mode->w, mode->h, mode->pixel_density, mode->refresh_rate);
        }
    }
}

void sdl_helper::print_opengl_info() const noexcept
{
     const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

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

void sdl_helper::set_window_icon(std::string_view icon_path) noexcept
{
    SDL_Surface *icon_surface = SDL_LoadBMP_IO(SDL_IOFromFile(icon_path.data(), "rb"), true);
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
