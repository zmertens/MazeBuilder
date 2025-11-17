#include "Pathfinder.hpp"

#include "ResourceManager.hpp"
#include "Texture.hpp"
#include "Wall.hpp"
#include "Player.hpp"

Pathfinder::Pathfinder(Type type, const TextureManager& textures)
    : mType(type)
      , mSprite(textures.get(Pathfinder::getTextureID()))
{
}

Category::Type Pathfinder::getCategory() const noexcept
{
    switch (mType)
    {
    case Type::ALLY:
        {
            return Category::Type::PLAYER;
        }
    case Type::ENEMY:
        {
            return Category::Type::ENEMY;
        }
    default:
        {
            return Category::Type::NONE;
        }
    }
}

void Pathfinder::updateCurrent(float dt, CommandQueue& commands) noexcept
{
    // Call base class to sync physics body position to scene node transform
    Entity::updateCurrent(dt, commands);

    // Additional Pathfinder-specific update logic can be added here
}

void Pathfinder::drawCurrent(SDL_Renderer* renderer, RenderStates states) const noexcept
{
    mSprite.draw(renderer, states);
}

Textures::ID Pathfinder::getTextureID() const noexcept
{
    switch (mType)
    {
    case Type::ALLY:
        {
            return Textures::ID::CHARACTER;
        }
    case Type::ENEMY:
        {
            return Textures::ID::CHARACTER;
        }
    default:
        {
            return Textures::ID::CHARACTER;
        }
    }
}
