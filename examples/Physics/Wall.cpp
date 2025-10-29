#include "Wall.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <random>

#include "OrthographicCamera.hpp"

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

void Wall::update(float elapsed) noexcept {
    // Update logic here
    // SDL_Log("Wall update() called - implement update logic here");
}

void Wall::draw(float elapsed) const noexcept {
    // Drawing code here
    // SDL_Log("Wall draw() called - implement rendering logic here");
}
