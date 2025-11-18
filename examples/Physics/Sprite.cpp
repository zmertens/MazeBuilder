#include "Sprite.hpp"

#include "Texture.hpp"

#include <SDL3/SDL.h>

Sprite::Sprite(const Texture& texture)
    : mTexture(&texture), mTextureRect{SDL_Rect{0, 0, texture.getWidth(), texture.getHeight()}}
{
}

Sprite::Sprite(const Texture& texture, const SDL_Rect& rect)
    : mTexture(&texture), mTextureRect{rect}
{
}

void Sprite::draw(SDL_Renderer* renderer, RenderStates states) const noexcept
{
    if (!mTexture)
    {
        return;
    }

    // Check if SDL is still initialized before accessing renderer
    if (!SDL_WasInit(SDL_INIT_VIDEO))
    {
        return; // SDL already quit, skip drawing
    }

    if (!renderer)
    {
        return; // Renderer not available
    }

    if (const auto* sdlTexture = mTexture->get(); !sdlTexture)
    {
        return;
    }

    auto [x, y, w, h] = SDL_Rect{0, 0, mTexture->getWidth(), mTexture->getHeight()};

    // Convert source rect from SDL_Rect to SDL_FRect
    SDL_FRect srcRect;
    srcRect.x = static_cast<float>(x);
    srcRect.y = static_cast<float>(y);
    srcRect.w = static_cast<float>(w);
    srcRect.h = static_cast<float>(h);

    // Use the transform from RenderStates (passed from SceneNode hierarchy)
    SDL_FRect dstRect;
    dstRect.x = states.transform.p.x;
    dstRect.y = states.transform.p.y;
    dstRect.w = static_cast<float>(w);
    dstRect.h = static_cast<float>(h);

    if (!SDL_RenderTexture(renderer, mTexture->get(), &srcRect, &dstRect))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_RenderTexture failed: %s", SDL_GetError());
    }
}

/// @brief
/// @param texture
/// @param resetRect false
void Sprite::setTexture(const Texture& texture, bool resetRect)
{
    mTexture = &texture;

    if (resetRect)
    {
        mTextureRect = SDL_Rect{0, 0, texture.getWidth(), texture.getHeight()};
    }
}
