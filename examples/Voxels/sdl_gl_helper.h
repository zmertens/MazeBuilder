#ifndef SDL_HELPER_H
#define SDL_HELPER_H

#include <mutex>
#include <string_view>
#include <utility>

#include <SDL3/SDL.h>

class player;
class scene_node;
struct SDL_Window;

class sdl_gl_helper
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

    void set_window_icon(std::string_view icon_path) const noexcept;

    [[nodiscard]] std::int32_t get_scale_factor() const noexcept;
    std::pair<std::int32_t, std::int32_t> get_window_size() const noexcept;
    std::pair<std::int32_t, std::int32_t> get_window_size_in_pixels() const noexcept;

    static void print_display_modes() noexcept;

    static void print_opengl_info() noexcept;

    static void del_buffer(std::uint32_t buffer) noexcept;
    static std::uint32_t gen_buffer(std::size_t size, const float* data) noexcept;
    static float* malloc_faces(std::size_t components, std::size_t faces) noexcept;
    static std::uint32_t gen_faces(std::size_t components, std::size_t faces, const float* data) noexcept;

    [[nodiscard]] std::uint32_t gen_crosshair_buffer() const noexcept;
    [[nodiscard]] static std::uint32_t gen_wireframe_buffer(float x, float y, float z, float n) noexcept;
    [[nodiscard]] static std::uint32_t gen_cube_buffer(float x, float y, float z, float n, int w) noexcept;
    [[nodiscard]] static std::uint32_t gen_plant_buffer(float x, float y, float z, float n, int w) noexcept;
    [[nodiscard]] static std::uint32_t gen_player_buffer(float x, float y, float z, float rx, float ry) noexcept;
    [[nodiscard]] static std::uint32_t gen_text_buffer(float x, float y, float n, std::string_view text) noexcept;
    static int _gen_sign_buffer(float* data, float x, float y, float z, int face, std::string_view text) noexcept;
    static void gen_sign_buffer(scene_node* chunk) noexcept;

    static void draw_triangles_3d_ao(const attrib* a, std::uint32_t buffer, int count) noexcept;
    static void draw_triangles_3d_text(const attrib* a, std::uint32_t buffer, int count) noexcept;
    static void draw_triangles_3d(const attrib* a, std::uint32_t buffer, int count) noexcept;
    static void draw_triangles_2d(const attrib* a, std::uint32_t buffer, std::size_t count) noexcept;
    static void draw_lines(const attrib* a, std::uint32_t buffer, int components, int count) noexcept;
    static void draw_chunk(const attrib* a, const scene_node* chunk) noexcept;
    static void draw_item(const attrib* a, std::uint32_t buffer, int count) noexcept;
    static void draw_text(const attrib* a, std::uint32_t buffer, std::size_t length) noexcept;
    static void draw_signs(const attrib* a, const scene_node* chunk) noexcept;
    static void draw_sign(const attrib* a, std::uint32_t buffer, int length) noexcept;
    static void draw_cube(const attrib* a, std::uint32_t buffer) noexcept;
    static void draw_plant(const attrib* a, std::uint32_t buffer) noexcept;
    static void draw_player(const attrib* a, const player* _player) noexcept;

private:
    std::once_flag m_initialized_flag;
};

#endif // SDL_HELPER_H
