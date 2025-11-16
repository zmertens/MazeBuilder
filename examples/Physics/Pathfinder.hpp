#ifndef PATHFINDER_HPP
#define PATHFINDER_HPP

#include "Category.hpp"
#include "Entity.hpp"
#include "RenderStates.hpp"
#include "ResourceIdentifiers.hpp"
#include "Sprite.hpp"

class Pathfinder final : public Entity
{
public:
    enum class Type
    {
        ALLY,
        ENEMY
    };

    explicit Pathfinder(Type type, const TextureManager& textures);

    ~Pathfinder() override = default;

    [[nodiscard]] Category::Type getCategory() const noexcept override;

private:
    void drawCurrent(SDL_Renderer* renderer, RenderStates states) const noexcept override;

    [[nodiscard]] Textures::ID getTextureID() const noexcept override;

    void updateCurrent(float dt, CommandQueue&) noexcept override;

    Type mType;
    Sprite mSprite;
};

#endif // PATHFINDER_HPP
