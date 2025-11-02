#include "Texture.hpp"

#include "SDLHelper.hpp"

#include <SDL3/SDL.h>

#include <MazeBuilder/enums.h>

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

bool Texture::loadFromStr(std::string_view str, int cellSize) noexcept {
    
    auto renderer = mazes::singleton_base<SDLHelper>::instance().get()->renderer;

    this->free();

    if (str.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Maze string is empty");
        return false;
    }

    // Parse the maze string to determine dimensions
    int mazeWidth = 0;
    int mazeHeight = 0;
    int currentLineWidth = 0;

    for (char c : str) {
        if (c == '\n') {
            mazeHeight++;
            if (currentLineWidth > mazeWidth) {
                mazeWidth = currentLineWidth;
            }
            currentLineWidth = 0;
        } else {
            currentLineWidth++;
        }
    }
    
    // Handle last line if it doesn't end with newline
    if (currentLineWidth > 0) {
        mazeHeight++;
        if (currentLineWidth > mazeWidth) {
            mazeWidth = currentLineWidth;
        }
    }

    if (mazeWidth == 0 || mazeHeight == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid maze dimensions: %dx%d", mazeWidth, mazeHeight);
        return false;
    }

    // Create a surface to render the maze onto
    this->width = mazeWidth * cellSize;
    this->height = mazeHeight * cellSize;

    SDL_Surface* surface = SDL_CreateSurface(this->width, this->height, SDL_PIXELFORMAT_RGBA8888);
    
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create surface for maze: %s", SDL_GetError());
        return false;
    }

    // Fill with white background
    SDL_FillSurfaceRect(surface, nullptr, SDL_MapSurfaceRGBA(surface, 255, 255, 255, 255));

    // Draw the maze character by character
    int row = 0;
    int col = 0;

    for (char c : str) {
        if (c == '\n') {
            row++;
            col = 0;
            continue;
        }

        // Determine color based on character
        SDL_Rect charRect = {col * cellSize, row * cellSize, cellSize, cellSize};
        
        std::uint8_t r = 255, g = 255, b = 255, a = 255;

        if (c == static_cast<char>(mazes::barriers::CORNER) 
            || c == static_cast<char>(mazes::barriers::HORIZONTAL) 
            || c == static_cast<char>(mazes::barriers::VERTICAL)) {

            // Walls are black
            r = 0; g = 0; b = 0;
        } else if (c == ' ') {

            // Paths are white (already the background color)
            r = 255; g = 255; b = 255;
        }

        SDL_FillSurfaceRect(surface, &charRect, SDL_MapSurfaceRGBA(surface, r, g, b, a));
        
        col++;
    }

    // Create texture from surface
    this->texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    if (!this->texture) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture from maze surface: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    SDL_SetTextureBlendMode(this->texture, SDL_BLENDMODE_BLEND);
    
    SDL_DestroySurface(surface);

    return true;
}
