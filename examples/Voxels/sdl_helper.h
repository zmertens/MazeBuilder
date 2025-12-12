#ifndef SDL_HELPER_H
#define SDL_HELPER_H

#include <mutex>
#include <string_view>

#include <SDL3/SDL.h>

#include "player.h"

class attrib;
class scene_node;
struct SDL_Window;

class sdl_helper
{
public:
    class attrib {
    public:
        std::uint32_t program;
        std::uint32_t position;
        std::uint32_t normal;
        std::uint32_t uv;
        std::uint32_t matrix;
        std::uint32_t sampler;
        std::uint32_t camera;
        std::uint32_t timer;
        std::uint32_t extra1;
        std::uint32_t extra2;
        std::uint32_t extra3;
        std::uint32_t extra4;
    };

    SDL_Window* window;
    SDL_GLContext gl_context;

    bool initialize(std::string_view title, int width, int height) noexcept;

    void destroy_and_quit() noexcept;

    void set_window_icon(std::string_view icon_path) noexcept;

    std::int32_t get_scale_factor() const noexcept;

    static void print_display_modes() noexcept;

    static void print_opengl_info() noexcept;

    static void del_buffer(std::uint32_t buffer) noexcept;

    static std::uint32_t gen_buffer(std::size_t size, const float* data) noexcept;

    static float* malloc_faces(std::size_t components, std::size_t faces) noexcept;

    static std::uint32_t gen_faces(std::size_t components, std::size_t faces, const float* data) noexcept;

    static void draw_triangles_3d_ao(const attrib* a, std::uint32_t buffer, int count) noexcept;
    static void draw_triangles_3d_text(const attrib* a, std::uint32_t buffer, int count) noexcept;
    void draw_triangles_3d(const attrib* a, std::uint32_t buffer, int count) const noexcept;
    void draw_triangles_2d(const attrib* a, std::uint32_t buffer, std::size_t count) const noexcept;
    void draw_lines(const attrib* a, std::uint32_t buffer, int components, int count) const noexcept;
    void draw_chunk(const attrib* a, const scene_node* chunk) const noexcept;
    void draw_item(const attrib* a, std::uint32_t buffer, int count) const noexcept;
    void draw_text(const attrib* a, std::uint32_t buffer, std::size_t length) const noexcept;
    void draw_signs(const attrib* a, const scene_node* chunk) const noexcept;
    void draw_sign(const attrib* a, std::uint32_t buffer, int length) const noexcept;
    void draw_cube(const attrib* a, std::uint32_t buffer) const noexcept;
    void draw_plant(const attrib* a, std::uint32_t buffer) const noexcept;
    void draw_player(const attrib* a, const player* _player) const noexcept;

private:
    std::once_flag m_initialized_flag;
};

#endif // SDL_HELPER_H
