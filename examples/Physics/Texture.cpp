#include "Texture.hpp"

#include "SDLHelper.hpp"

#include <SDL3/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Texture::Texture() noexcept : texture{ nullptr }, width(0), height(0) {
}

Texture::~Texture() {

    this->free();
}

void Texture::free() noexcept {
    if (this->texture) {

        SDL_DestroyTexture(texture);
    }
}

SDL_Texture* Texture::get() const noexcept {

    return this->texture;
}

bool Texture::loadTarget(int w, int h) noexcept {

    auto renderer = mazes::singleton_base<SDLHelper>::instance().get()->renderer;

    this->free();

    this->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    
    if (!this->texture) {
    
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture: %s\n", SDL_GetError());
    } else {
    
        this->width = w;
        this->height = h;
    }

    return this->texture != nullptr;
}

// Load an image file using stb_image and create an SDL texture
bool Texture::loadFromFile(std::string_view path) noexcept {

    auto renderer = mazes::singleton_base<SDLHelper>::instance().get()->renderer;

    this->free();

    int width, height, channels;
    unsigned char* imageData = stbi_load(path.data(), &width, &height, &channels, STBI_rgb_alpha);
    
    if (!imageData) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load image: %s - %s\n", path.data(), stbi_failure_reason());

        return false;
    }
    
    // Create surface from image data (force RGBA format)
    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA8888, imageData, width * 4);

    if (!surface) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create surface: %s\n", SDL_GetError());

        stbi_image_free(imageData);
        
        SDL_DestroySurface(surface);

        return false;
    }
    
    // Create texture from surface
    this->texture = SDL_CreateTextureFromSurface(renderer, surface);

    if (!this->texture) {

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

bool Texture::loadImageTexture(std::string_view imagePath) noexcept {

    auto renderer = mazes::singleton_base<SDLHelper>::instance().get()->renderer;

    this->free();

    SDL_Surface* loadedSurface = SDL_LoadBMP(imagePath.data());
    if (!loadedSurface) {
    
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to load image %s! SDL Error: %s\n", imagePath.data(), SDL_GetError());
        return false;
    }
    if (!loadedSurface) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to load image %s! SDL Error: %s\n", imagePath.data(), SDL_GetError());
        return false;
    }

    this->texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    
    if (!this->texture) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unable to create texture from %s! SDL Error: %s\n", imagePath.data(), SDL_GetError());
        SDL_DestroySurface(loadedSurface);
        return false;
    }

    this->width = loadedSurface->w;
    this->height = loadedSurface->h;

    SDL_DestroySurface(loadedSurface);
    
    return true;
}

void Texture::render(int x, int y) const noexcept {

    auto renderer = mazes::singleton_base<SDLHelper>::instance().get()->renderer;

    SDL_FRect destinationRect = { static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(width),
        static_cast<float>(height) };

    SDL_RenderTexture(renderer, this->texture, nullptr, &destinationRect);
}
