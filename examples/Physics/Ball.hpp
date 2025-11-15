#ifndef BALL_HPP
#define BALL_HPP

#include "Entity.hpp"
#include "ResourceIdentifiers.hpp"
#include "Sprite.hpp"

/// @file Ball.hpp
/// @class Ball
/// @brief Data class for a ball with physics properties
class Ball : public Entity
{
public:
    enum class Type
    {
        NORMAL,
        HEAVY,
        LIGHT,
        EXPLOSIVE
    };

    explicit Ball(Type type, const TextureManager& textureManager);

private:
    void drawCurrent(SDL_Renderer* renderer, RenderStates states) const noexcept override;

    Textures::ID getTextureID() const noexcept override;

    void updateCurrent(float dt) noexcept override;

    Type mType;
    Sprite mSprite;
};

#endif // BALL_HPP
