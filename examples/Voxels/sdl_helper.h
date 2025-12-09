#ifndef SDL_HELPER_H
#define SDL_HELPER_H

#include <mutex>
#include <string_view>

#include <SDL3/SDL.h>

struct SDL_Window;

class sdl_helper
{
public:
    SDL_Window* window;
    SDL_GLContext gl_context;

    bool initialize(std::string_view title, int width, int height) noexcept;

    void destroy_and_quit() noexcept;

    std::int32_t get_scale_factor() const noexcept;

    static void print_display_modes() noexcept;

    static void print_opengl_info() noexcept;

    void set_window_icon(std::string_view icon_path) noexcept;

    static void del_buffer(std::uint32_t buffer) noexcept;

    static std::uint32_t gen_buffer(std::size_t size, const float* data) noexcept;

    static float* malloc_faces(std::size_t components, std::size_t faces) noexcept;

    static std::uint32_t gen_faces(std::size_t components, std::size_t faces, const float* data) noexcept;

private:
    std::once_flag m_initialized_flag;
};

#endif // SDL_HELPER_H
