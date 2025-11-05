#ifndef PATHFINDER_HPP
#define PATHFINDER_HPP

#include "Category.hpp"
#include "Entity.hpp"
#include "RenderStates.hpp"
#include "ResourceIdentifiers.hpp"
#include "Sprite.hpp"

class Pathfinder : public Entity
{
public:
    enum class Type
    {
        ALLY,
        ENEMY
    };

    explicit Pathfinder(Type type, const TextureManager& textures);

    virtual ~Pathfinder() = default;

    virtual Category::Type getCategory() const noexcept override;

private:
    virtual void drawCurrent(SDL_Renderer* renderer, RenderStates states) const noexcept override;

    Textures::ID getTextureID() const noexcept override;

    virtual void updateCurrent(float dt) noexcept override;

    Type mType;
    Sprite mSprite;
};

#endif // PATHFINDER_HPP
