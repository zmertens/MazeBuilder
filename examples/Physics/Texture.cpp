#include "Texture.hpp"

#include <SDL3/SDL.h>

#include <MazeBuilder/enums.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "MazeLayout.hpp"

void Texture::free() noexcept
{
    if (this->texture)
    {
#if defined(MAZE_DEBUG)

        SDL_Log("Texture::free - Freeing texture resource\n");
#endif

        SDL_DestroyTexture(texture);
        texture = nullptr;
        width = 0;
        height = 0;
    }
}

SDL_Texture* Texture::get() const noexcept
{
    return this->texture;
}

bool Texture::loadTarget(SDL_Renderer* renderer, int w, int h) noexcept
{
    this->free();

    this->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);

    if (!this->texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture: %s\n", SDL_GetError());
    }
    else
    {
        this->width = w;
        this->height = h;
    }

    return this->texture != nullptr;
}

// Load an image file using stb_image and create an SDL texture
bool Texture::loadFromFile(SDL_Renderer* renderer, std::string_view path) noexcept
{
    this->free();

    int width, height, channels;
    unsigned char* imageData = stbi_load(path.data(), &width, &height, &channels, STBI_rgb_alpha);

    if (!imageData)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load image: %s - %s\n", path.data(), stbi_failure_reason());

        return false;
    }

    // Create surface from image data (force RGBA format)
    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA8888, imageData, width * 4);

    if (!surface)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create surface: %s\n", SDL_GetError());

        stbi_image_free(imageData);

        SDL_DestroySurface(surface);

        return false;
    }

    // Create texture from surface
    this->texture = SDL_CreateTextureFromSurface(renderer, surface);

    if (!this->texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture: %s\n", SDL_GetError());

        return false;
    }

    // Set blend mode for transparency
    SDL_SetTextureBlendMode(this->texture, SDL_BLENDMODE_BLEND);

    // Clean up
    SDL_DestroySurface(surface);
    stbi_image_free(imageData);

    this->width = width;
    this->height = height;

    SDL_Log("Texture loaded successfully: %dx%d from %s", width, height, path.data());

    return true;
}

bool Texture::loadImageTexture(SDL_Renderer* renderer, std::string_view imagePath) noexcept
{
    this->free();

    if (SDL_Surface* loadedSurface = SDL_LoadBMP(imagePath.data())) {

        this->texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);

        if (!this->texture)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture from %s! SDL Error: %s\n", imagePath.data(),
                SDL_GetError());
            SDL_DestroySurface(loadedSurface);
            return false;
        }

        this->width = loadedSurface->w;
        this->height = loadedSurface->h;

        SDL_DestroySurface(loadedSurface);
    } else {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to load image %s! SDL Error: %s\n", imagePath.data(),
            SDL_GetError());

        return false;
    }

    return true;
}

bool Texture::loadFromStr(SDL_Renderer* renderer, std::string_view str, int cellSize) noexcept
{
    this->free();

    if (str.empty())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Maze string is empty");
        return false;
    }

    MazeLayout maze = MazeLayout::fromString(str, cellSize);

    if (maze.getRows() == 0 || maze.getColumns() == 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid maze generated from string");
        return false;
    }

    return loadFromMaze(renderer, maze);
}

bool Texture::loadFromMaze(SDL_Renderer* renderer, const MazeLayout& maze) noexcept
{
    this->free();

    if (maze.getRows() <= 0 || maze.getColumns() <= 0 || maze.getCellSize() <= 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "MazeLayout has invalid dimensions");
        return false;
    }

    SDL_Surface* surface = maze.buildSurface();

    if (!surface)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to build surface from MazeLayout");
        return false;
    }

    this->texture = SDL_CreateTextureFromSurface(renderer, surface);

    if (!this->texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture from maze surface: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    SDL_SetTextureBlendMode(this->texture, SDL_BLENDMODE_BLEND);

    this->width = maze.getPixelWidth();
    this->height = maze.getPixelHeight();

    SDL_DestroySurface(surface);

    return true;
}
