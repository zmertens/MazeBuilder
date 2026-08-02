#ifndef TEXTURE_H
#define TEXTURE_H

#include <cstdint>
#include <string_view>

struct SDL_Window;

/// @file texture.h
/// @brief texture class for SDL3/OpenGL
/// @details This class wraps OpenGL texture requirements
class texture final
{
public:
    static constexpr int MAX_TEXTURE_WIDTH = 1980;
    static constexpr int MAX_TEXTURE_HEIGHT = 1020;

    std::uint32_t gl_texture;

    std::uint32_t width;
    std::uint32_t height;

    std::uint8_t *pixels;

    texture() = default;

    ~texture() noexcept;

    texture(const texture &) = delete;
    texture &operator=(const texture &) = delete;

    texture(texture &&other) noexcept;
    texture &operator=(texture &&other) noexcept;

    void free() noexcept;

    bool load_target(int w, int h) noexcept;

    bool load_from_file(std::string_view filepath, std::uint32_t channel_offset = 0) noexcept;

    bool load_from_memory(const std::uint8_t *data, int width, int height, std::uint32_t channel_offset = 0, bool rotate_180 = false) noexcept;

    bool update_from_memory(const std::uint8_t *data, int width, int height, std::uint32_t channel_offset = 0, bool rotate_180 = false) noexcept;

    static bool load_bmp_icon(SDL_Window *window, std::string_view filepath) noexcept;
}; // texture class

#endif // TEXTURE_H
