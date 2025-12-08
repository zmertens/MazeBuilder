#ifndef TEXTURE_H
#define TEXTURE_H

#include <string_view>

struct SDL_Window;

/// @file texture.h
/// @brief texture class for SDL3/OpenGL
/// @details This class wraps OpenGL texture requirements
class texture
{
public:
    texture() = default;

    ~texture() noexcept;

    texture(const texture&) = delete;
    texture& operator=(const texture&) = delete;

    texture(texture&& other) noexcept;
    texture& operator=(texture&& other) noexcept;

    void free() noexcept;

    [[nodiscard]] std::uint32_t get() const noexcept;

    [[nodiscard]] int get_width() const noexcept;

    [[nodiscard]] int get_height() const noexcept;

    bool load_target(int w, int h) noexcept;

    bool load_from_file(std::string_view filepath, std::uint32_t channel_offset = 0) noexcept;

    static bool load_bmp_icon(SDL_Window* window,std::string_view filepath) noexcept;

private:
    std::uint32_t m_texture;
    int m_width;
    int m_height;
}; // texture class

#endif // TEXTURE_H
