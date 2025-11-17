#include "MazeNode.hpp"

#include "Sprite.hpp"
#include "RenderStates.hpp"

#include <SDL3/SDL.h>

MazeNode::MazeNode(const Texture& texture)
    : mSprite(texture)
{
}

void MazeNode::drawCurrent(SDL_Renderer* renderer, RenderStates states) const noexcept
{
    mSprite.draw(renderer, states);
}

