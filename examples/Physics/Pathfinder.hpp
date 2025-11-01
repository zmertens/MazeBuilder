#ifndef PATHFINDER_HPP
#define PATHFINDER_HPP

#include "Entity.hpp"
#include "RenderStates.hpp"
#include "ResourceIdentifiers.hpp"
#include "Sprite.hpp"

class Pathfinder : public Entity {
public:

    enum class Type {
        ALLY,
        ENEMY
    };

    explicit Pathfinder(Type type, const TextureManager& textures);

    virtual Type getCategory() const noexcept;


private:
    virtual void drawCurrent(RenderStates states) const noexcept override;

    Textures::ID toTextureID(Type type) const noexcept;


private:
    Type mType;
    Sprite mSprite;
};

#endif // PATHFINDER_HPP
