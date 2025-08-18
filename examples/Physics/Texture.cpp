#include "Texture.hpp"

#include <SDL3/SDL.h>

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

bool Texture::loadTarget(SDL_Renderer* renderer, int w, int h) {
    
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

void Texture::render(SDL_Renderer *renderer, int x, int y) const noexcept {
    
    SDL_FRect renderQuad = { static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(width),
        static_cast<float>(height) };

    SDL_RenderTexture(renderer, texture, nullptr, &renderQuad);
}
