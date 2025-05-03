#ifndef WALL_HPP
#define WALL_HPP

#include <box2d/box2d.h>

class Wall {
public:
    enum class Orientation {
        HORIZONTAL, VERTICAL, CORNER
    };

    // Constructor
    explicit Wall(b2BodyId bodyId, b2ShapeId shapeId, int hitCount, bool isDestroyed, int row, int col, Orientation orientation);

    // Getters
    b2BodyId getBodyId() const;
    b2ShapeId getShapeId() const;
    int getHitCount() const;
    bool getIsDestroyed() const;
    int getRow() const;
    int getCol() const;
    Orientation getOrientation() const;

    // Setters
    void setBodyId(const b2BodyId& id);
    void setShapeId(const b2ShapeId& id);
    void setHitCount(int hitCount);
    void setIsDestroyed(bool isDestroyed);
    void setRow(int row);
    void setCol(int col);
    void setOrientation(Orientation orientation);

private:
    b2BodyId bodyId;
    b2ShapeId shapeId;
    int hitCount;
    bool isDestroyed;
    int row;
    int col;
    Orientation orientation;

};

#endif // WALL_HPP
