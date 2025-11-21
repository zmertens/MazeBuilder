#include "texture.h"

#include <SDL3/SDL.h>

#include <MazeBuilder/enums.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

texture::texture(texture&& other) noexcept : m_texture(other.m_texture), m_width(other.m_width),
                                             m_height(other.m_height)
{
    other.m_texture = nullptr;
    other.m_width = 0;
    other.m_height = 0;
}

texture& texture::operator=(texture&& other) noexcept
{
    if (this != &other)
    {
        free(); // Clean up existing texture
        m_texture = other.m_texture;
        m_width = other.m_width;
        m_height = other.m_height;
        other.m_texture = nullptr;
        other.m_width = 0;
        other.m_height = 0;
    }
    return *this;
}

void texture::free() noexcept
{
    if (this->m_texture)
    {
#if defined(MAZE_DEBUG)

        SDL_Log("Texture::free - Freeing texture resource\n");
#endif

        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
        m_width = 0;
        m_height = 0;
    }
}

SDL_Texture* texture::get() const noexcept
{
    return this->m_texture;
}

bool texture::loadTarget(SDL_Renderer* renderer, int w, int h) noexcept
{
    this->free();

    this->m_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);

    if (!this->m_texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture: %s\n", SDL_GetError());
    }
    else
    {
        this->m_width = w;
        this->m_height = h;
    }

    return this->m_texture != nullptr;
}

// Load an image file using stb_image and create an SDL texture
bool texture::loadFromFile(SDL_Renderer* renderer, std::string_view path) noexcept
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
    this->m_texture = SDL_CreateTextureFromSurface(renderer, surface);

    if (!this->m_texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture: %s\n", SDL_GetError());

        return false;
    }

    // Set blend mode for transparency
    SDL_SetTextureBlendMode(this->m_texture, SDL_BLENDMODE_BLEND);

    // Clean up
    SDL_DestroySurface(surface);
    stbi_image_free(imageData);

    this->m_width = width;
    this->m_height = height;

    SDL_Log("Texture loaded successfully: %dx%d from %s", width, height, path.data());

    return true;
}

bool texture::loadImageTexture(SDL_Renderer* renderer, std::string_view imagePath) noexcept
{
    this->free();

    if (SDL_Surface* loadedSurface = SDL_LoadBMP(imagePath.data()))
    {
        this->m_texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);

        if (!this->m_texture)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture from %s! SDL Error: %s\n", imagePath.data(),
                         SDL_GetError());
            SDL_DestroySurface(loadedSurface);
            return false;
        }

        this->m_width = loadedSurface->w;
        this->m_height = loadedSurface->h;

        SDL_DestroySurface(loadedSurface);
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to load image %s! SDL Error: %s\n", imagePath.data(),
                     SDL_GetError());

        return false;
    }

    return true;
}

