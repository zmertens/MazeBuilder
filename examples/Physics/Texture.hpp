#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <string_view>

struct SDL_Texture;

/// @file Texture.hpp
/// @brief Texture class for SDL3
/// @details This class wraps SDL_Texture and provides methods for loading, rendering, and freeing textures.
class Texture {
public:
    explicit Texture() noexcept;

    ~Texture();

    Texture(const Texture& other) = delete;

    Texture& operator=(const Texture& other) = delete;

    Texture(Texture&& other) noexcept = delete;

    Texture& operator=(Texture&& other) noexcept = delete;

    void free() noexcept;

    SDL_Texture* get() const noexcept;

    int getWidth() const noexcept { return width; }
    
    int getHeight() const noexcept { return height; }

    bool loadTarget(int w, int h) noexcept;
    
    bool loadFromFile(std::string_view path) noexcept;

    bool loadImageTexture(std::string_view imagePath) noexcept;

    bool loadFromStr(std::string_view str, int cellSize = 10) noexcept;
private:
    SDL_Texture* texture;
    int width, height;
}; // Texture class

#endif // TEXTURE_HPP
