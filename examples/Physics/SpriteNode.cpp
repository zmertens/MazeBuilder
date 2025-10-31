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

void SpriteNode::draw(RenderStates states) const noexcept
{
    drawCurrent(states);
}

void SpriteNode::drawCurrent(RenderStates states) const noexcept
{
    mSprite.draw(states);
}
