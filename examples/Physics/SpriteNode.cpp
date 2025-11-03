#include "SpriteNode.hpp"

#include "Sprite.hpp"
#include "RenderStates.hpp"

#include <SDL3/SDL.h>


SpriteNode::SpriteNode(const Texture& texture)
    : mSprite(texture)
{
}

SpriteNode::SpriteNode(const Texture& texture, const SDL_Rect& textureRect)
    : mSprite(texture, textureRect)
{
}

void SpriteNode::drawCurrent(SDL_Renderer* renderer, RenderStates states) const noexcept
{
    mSprite.draw(renderer, states);
}
