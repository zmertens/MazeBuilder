#include "ParallaxNode.hpp"

#include "Texture.hpp"
#include "RenderStates.hpp"

#include <SDL3/SDL.h>

ParallaxNode::ParallaxNode(const Texture& texture, float scrollSpeed)
    : mTexture(texture)
    , mScrollOffset(0.0f)
    , mScrollSpeed(scrollSpeed)
    , mVerticalOffset(0.0f)
    , mScale(1.0f)
{
}

void ParallaxNode::updateCurrent(float dt, CommandQueue& commands) noexcept
{
    // Update scrolling offset based on speed and delta time
    mScrollOffset += mScrollSpeed * dt;

    // Reset offset when it exceeds texture width to create seamless loop
    // NOTE: Texture is scaled, so we need to account for that
    float scaledWidth = static_cast<float>(mTexture.getWidth()) * mScale;

    if (mScrollSpeed < 0.0f)
    {
        // Scrolling left
        if (mScrollOffset <= -scaledWidth)
            mScrollOffset = 0.0f;
    }
    else if (mScrollSpeed > 0.0f)
    {
        // Scrolling right
        if (mScrollOffset >= scaledWidth)
            mScrollOffset = 0.0f;
    }
}

void ParallaxNode::drawCurrent(SDL_Renderer* renderer, RenderStates states) const noexcept
{
    if (!mTexture.get())
        return;

    SDL_Texture* texture = mTexture.get();

    // Calculate the scaled dimensions
    float scaledWidth = static_cast<float>(mTexture.getWidth()) * mScale;
    float scaledHeight = static_cast<float>(mTexture.getHeight()) * mScale;

    // Apply transform from states and node position
    const b2Vec2& position = getPosition();
    float totalX = position.x + mScrollOffset;
    float totalY = position.y + mVerticalOffset;

    // Create destination rectangles for the two texture draws
    SDL_FRect destRect1 = {
        totalX,
        totalY,
        scaledWidth,
        scaledHeight
    };

    SDL_FRect destRect2 = {
        totalX + scaledWidth,
        totalY,
        scaledWidth,
        scaledHeight
    };

    // Draw the texture twice to create seamless scrolling
    // First instance
    SDL_RenderTexture(renderer, texture, nullptr, &destRect1);

    // Second instance (adjacent to first)
    SDL_RenderTexture(renderer, texture, nullptr, &destRect2);

    // If scrolling left and we're far enough, draw a third copy on the left
    if (mScrollSpeed < 0.0f && mScrollOffset < -scaledWidth * 0.5f)
    {
        SDL_FRect destRect0 = {
            totalX - scaledWidth,
            totalY,
            scaledWidth,
            scaledHeight
        };
        SDL_RenderTexture(renderer, texture, nullptr, &destRect0);
    }
}

