#include "Wall.hpp"

#include <algorithm>
#include <cmath>
#include <random>

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

void Wall::updateCurrent(float dt) {
    // Walls do not have children to update
}

void Wall::draw(RenderStates states) const noexcept {
    if (!isDestroyed) {
        Entity::draw(states);
    }
}
