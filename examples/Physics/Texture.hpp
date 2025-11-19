#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <string_view>

struct SDL_Texture;
struct SDL_Renderer;

class MazeLayout;

/// @file Texture.hpp
/// @brief Texture class for SDL3
/// @details This class wraps SDL_Texture and provides methods for loading, rendering, and freeing textures.
class Texture
{
public:
    Texture() = default;

    // Destructor to ensure SDL texture is properly freed
    ~Texture() noexcept { free(); }

    // Delete copy constructor and copy assignment (textures shouldn't be copied)
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Allow move semantics if needed in the future
    Texture(Texture&& other) noexcept : texture(other.texture), width(other.width), height(other.height)
    {
        other.texture = nullptr;
        other.width = 0;
        other.height = 0;
    }

    Texture& operator=(Texture&& other) noexcept
    {
        if (this != &other)
        {
            free(); // Clean up existing texture
            texture = other.texture;
            width = other.width;
            height = other.height;
            other.texture = nullptr;
            other.width = 0;
            other.height = 0;
        }
        return *this;
    }

    void free() noexcept;

    [[nodiscard]] SDL_Texture* get() const noexcept;

    [[nodiscard]] int getWidth() const noexcept { return width; }

    [[nodiscard]] int getHeight() const noexcept { return height; }

    bool loadTarget(SDL_Renderer* renderer, int w, int h) noexcept;

    bool loadFromFile(SDL_Renderer* renderer, std::string_view path) noexcept;

    bool loadImageTexture(SDL_Renderer* renderer, std::string_view imagePath) noexcept;

    bool loadFromStr(SDL_Renderer* renderer, std::string_view str, int cellSize = 10) noexcept;

    bool loadFromMaze(SDL_Renderer* renderer, const MazeLayout& maze) noexcept;

private:
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
}; // Texture class

#endif // TEXTURE_HPP
