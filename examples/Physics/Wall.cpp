#include "Wall.hpp"

#include "CommandQueue.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"

Wall::Wall(Orientation orientation, const TextureManager& textureManager)
    : hitCount{0}
      , isDestroyed{false}
      , row{0}
      , col{0}
      , orientation{orientation}
      , mSprite{textureManager.get(getTextureID())}
{
}

int Wall::getHitCount() const { return hitCount; }
bool Wall::getIsDestroyed() const { return isDestroyed; }
int Wall::getRow() const { return row; }
int Wall::getCol() const { return col; }
Wall::Orientation Wall::getOrientation() const { return orientation; }

// Setters
void Wall::setHitCount(int hitCount) { this->hitCount = hitCount; }
void Wall::setIsDestroyed(bool isDestroyed) { this->isDestroyed = isDestroyed; }
void Wall::setRow(int row) { this->row = row; }
void Wall::setCol(int col) { this->col = col; }
void Wall::setOrientation(Orientation orientation) { this->orientation = orientation; }

void Wall::updateCurrent(float dt, CommandQueue& commands) noexcept
{
    // Call base class to sync physics body position to scene node transform
    Entity::updateCurrent(dt, commands);
}

void Wall::drawCurrent(SDL_Renderer* renderer, RenderStates states) const noexcept
{
    if (!isDestroyed)
    {
        mSprite.draw(renderer, states);
    }
}

Textures::ID Wall::getTextureID() const noexcept
{
    switch (orientation)
    {
    case Orientation::HORIZONTAL:
        return Textures::ID::WALL_HORIZONTAL;
    case Orientation::VERTICAL:
        return Textures::ID::WALL_HORIZONTAL;
    case Orientation::CORNER:
        return Textures::ID::WALL_HORIZONTAL;
    default:
        return Textures::ID::WALL_HORIZONTAL;
    }
}
