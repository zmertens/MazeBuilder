#ifndef WALL_HPP
#define WALL_HPP

#include "Entity.hpp"
#include "ResourceIdentifiers.hpp"
#include "Sprite.hpp"

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

private:
    virtual void drawCurrent(RenderStates states) const noexcept override;

private:

    Textures::ID getTextureID() const noexcept override;

    virtual void updateCurrent(float dt) noexcept override;

    int hitCount;
    bool isDestroyed;
    int row;
    int col;
    Orientation orientation;
    Sprite mSprite;

};

#endif // WALL_HPP
