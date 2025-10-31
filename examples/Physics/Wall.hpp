#ifndef WALL_HPP
#define WALL_HPP

#include "Entity.hpp"
#include "ResourceIdentifiers.hpp"

class Wall : public Entity {
public:
    enum class Orientation {
        HORIZONTAL, VERTICAL, CORNER
    };

    explicit Wall(Orientation orientation, const TextureManager& textureManager);

    // Getters
    int getHitCount() const;
    bool getIsDestroyed() const;
    int getRow() const;
    int getCol() const;
    Orientation getOrientation() const;

    // Setters
    void setHitCount(int hitCount);
    void setIsDestroyed(bool isDestroyed);
    void setRow(int row);
    void setCol(int col);
    void setOrientation(Orientation orientation);

    void draw(RenderStates states) const noexcept;

private:

    virtual void updateCurrent(float dt) override;

    int hitCount;
    bool isDestroyed;
    int row;
    int col;
    Orientation orientation;

};

#endif // WALL_HPP
