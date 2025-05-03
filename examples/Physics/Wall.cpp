#include "Wall.hpp"

// Constructor
Wall::Wall(b2BodyId bodyId, b2ShapeId shapeId, int hitCount, bool isDestroyed, int row, int col, Orientation orientation)
    : bodyId(bodyId), shapeId(shapeId), hitCount(hitCount), isDestroyed(isDestroyed), row(row), col(col), orientation(orientation) {
}

// Getters
b2BodyId Wall::getBodyId() const { return bodyId; }
b2ShapeId Wall::getShapeId() const { return shapeId; }
int Wall::getHitCount() const { return hitCount; }
bool Wall::getIsDestroyed() const { return isDestroyed; }
int Wall::getRow() const { return row; }
int Wall::getCol() const { return col; }
Wall::Orientation Wall::getOrientation() const { return orientation; }

// Setters
void Wall::setBodyId(const b2BodyId& id) { bodyId = id; }
void Wall::setShapeId(const b2ShapeId& id) { shapeId = id; }
void Wall::setHitCount(int hitCount) { this->hitCount = hitCount; }
void Wall::setIsDestroyed(bool isDestroyed) { this->isDestroyed = isDestroyed; }
void Wall::setRow(int row) { this->row = row; }
void Wall::setCol(int col) { this->col = col; }
void Wall::setOrientation(Orientation orientation) { this->orientation = orientation; }
