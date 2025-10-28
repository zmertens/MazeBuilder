#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <string_view>

struct SDL_Texture;
struct SDL_Renderer;

/// @file Texture.hpp
/// @brief Texture class for SDL3
/// @details This class wraps SDL_Texture and provides methods for loading, rendering, and freeing textures.
class Texture {
private:
    SDL_Texture* texture;
    int width, height;
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

    bool loadTarget(SDL_Renderer* renderer, int w, int h) noexcept;
    
    bool loadFromFile(SDL_Renderer* renderer, std::string_view path) noexcept;

    bool loadImageTexture(SDL_Renderer* renderer, std::string_view imagePath) noexcept;

    void render(SDL_Renderer *renderer, int x, int y) const noexcept;
}; // Texture class

#endif // TEXTURE_HPP
