#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <string_view>

struct SDL_Texture;
struct SDL_Renderer;

/// @file texture.h
/// @brief Texture class for SDL3
/// @details This class wraps SDL_Texture and provides methods for loading, rendering, and freeing textures.
class texture
{
public:
    texture() = default;

    ~texture() noexcept { free(); }

    texture(const texture&) = delete;
    texture& operator=(const texture&) = delete;

    texture(texture&& other) noexcept;

    texture& operator=(texture&& other) noexcept;

    void free() noexcept;

    [[nodiscard]] SDL_Texture* get() const noexcept;

    [[nodiscard]] int getWidth() const noexcept { return m_width; }

    [[nodiscard]] int getHeight() const noexcept { return m_height; }

    bool loadTarget(SDL_Renderer* renderer, int w, int h) noexcept;

    bool loadFromFile(SDL_Renderer* renderer, std::string_view path) noexcept;

    bool loadImageTexture(SDL_Renderer* renderer, std::string_view imagePath) noexcept;

private:
    SDL_Texture* m_texture;
    int m_width;
    int m_height;
}; // Texture class

#endif // TEXTURE_HPP
