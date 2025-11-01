#include "Wall.hpp"

#include <algorithm>
#include <cmath>
#include <random>

#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"

Wall::Wall(Orientation orientation, const TextureManager& textureManager)
    : hitCount{ 0 }
    , isDestroyed{ false }
    , row{ 0 }
    , col{ 0 }
    , orientation{ orientation }
    , mSprite{ textureManager.get(getTextureID()) } {
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

void Wall::updateCurrent(float dt) noexcept {
    // Walls do not have children to update
}

void Wall::drawCurrent(RenderStates states) const noexcept {
    if (!isDestroyed) {
        mSprite.draw(states);
    }
}

Textures::ID Wall::getTextureID() const noexcept {
    
    switch (orientation) {
    case Orientation::HORIZONTAL:
        return Textures::ID::AVATAR;
    case Orientation::VERTICAL:
        return Textures::ID::AVATAR;
    case Orientation::CORNER:
        return Textures::ID::AVATAR;
    default:
        return Textures::ID::AVATAR;
    }
}