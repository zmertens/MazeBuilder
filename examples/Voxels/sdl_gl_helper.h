#ifndef SDL_GL_HELPER_H
#define SDL_GL_HELPER_H

#include <cstdint>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>

class player;
class scene_node;
struct SDL_Window;

class sdl_gl_helper
{
public:
    class attrib
    {
    public:
        int program{-1};
        int position{-1};
        int normal{-1};
        int uv{-1};
        int matrix{-1};
        int sampler{-1};
        int camera{-1};
        int timer{-1};
        int extra1{-1};
        int extra2{-1};
        int extra3{-1};
        int extra4{-1};
    };

    SDL_Window *window;
    SDL_GLContext gl_context;

    [[nodiscard]] bool initialize(std::string_view title, int width, int height) noexcept;

    void destroy_and_quit() noexcept;

    void set_window_icon(std::string_view icon_path) const noexcept;

    [[nodiscard]] std::int32_t get_scale_factor() const noexcept;
    [[nodiscard]] std::pair<std::int32_t, std::int32_t> get_window_size() const noexcept;
    [[nodiscard]] std::pair<std::int32_t, std::int32_t> get_window_size_in_pixels() const noexcept;

    static void print_display_modes() noexcept;
    static void print_opengl_info() noexcept;

    [[nodiscard]] static std::string load_file_to_string(std::string_view path) noexcept;
    [[nodiscard]] static std::vector<std::uint8_t> load_file_binary(std::string_view path) noexcept;

    static void del_buffer(std::uint32_t buffer) noexcept;
    [[nodiscard]] static std::uint32_t gen_buffer(std::size_t size, const float *data) noexcept;
    [[nodiscard]] static float *malloc_faces(std::size_t components, std::size_t faces) noexcept;
    static std::uint32_t gen_faces(std::size_t components, std::size_t faces, const float *data) noexcept;

    [[nodiscard]] std::uint32_t gen_crosshair_buffer() const noexcept;
    [[nodiscard]] static std::uint32_t gen_wireframe_buffer(float x, float y, float z, float n) noexcept;
    [[nodiscard]] static std::uint32_t gen_line_buffer(float x1, float y1, float z1,
                                                       float x2, float y2, float z2) noexcept;
    [[nodiscard]] static std::uint32_t gen_cube_buffer(float x, float y, float z, float n, int w) noexcept;
    [[nodiscard]] static std::uint32_t gen_plant_buffer(float x, float y, float z, float n, int w) noexcept;
    [[nodiscard]] static std::uint32_t gen_player_buffer(float x, float y, float z, float rx, float ry) noexcept;
    [[nodiscard]] static std::uint32_t gen_text_buffer(float x, float y, float n, std::string_view text) noexcept;
    [[nodiscard]] static int _gen_sign_buffer(float *data, float x, float y, float z, int face, std::string_view text) noexcept;
    static void gen_sign_buffer(scene_node *chunk) noexcept;
    [[nodiscard]] static std::uint32_t gen_sky_buffer() noexcept;

    static void draw_triangles_3d_ao(const attrib *a, std::uint32_t buffer, int count) noexcept;
    static void draw_triangles_3d_text(const attrib *a, std::uint32_t buffer, int count) noexcept;
    static void draw_triangles_3d(const attrib *a, std::uint32_t buffer, int count) noexcept;
    static void draw_triangles_2d(const attrib *a, std::uint32_t buffer, std::size_t count) noexcept;
    static void draw_lines(const attrib *a, std::uint32_t buffer, int components, int count) noexcept;
    static void draw_chunk(const attrib *a, const scene_node *chunk) noexcept;
    static void draw_item(const attrib *a, std::uint32_t buffer, int count) noexcept;
    static void draw_text(const attrib *a, std::uint32_t buffer, std::size_t length) noexcept;
    static void draw_signs(const attrib *a, const scene_node *chunk) noexcept;
    static void draw_sign(const attrib *a, std::uint32_t buffer, int length) noexcept;
    static void draw_cube(const attrib *a, std::uint32_t buffer) noexcept;
    static void draw_plant(const attrib *a, std::uint32_t buffer) noexcept;
    static void draw_player(const attrib *a, std::uint32_t buffer) noexcept;

private:
    std::once_flag m_initialized_flag;
};

#endif // SDL_GL_HELPER_H
