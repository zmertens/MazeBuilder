#include "Pathfinder.hpp"

#include "ResourceManager.hpp"
#include "Texture.hpp"

Pathfinder::Pathfinder(Type type, const TextureManager& textures)
    : mType(type)
    , mSprite(textures.get(getTextureID()))
{
}

Category::Type Pathfinder::getCategory() const noexcept
{
    switch (mType) {
        case Type::ALLY: {
            return Category::Type::PLAYER;
        }
        case Type::ENEMY: {
            return Category::Type::ENEMY;
        }
        default: {
            return Category::Type::NONE;
        }
    }
}

void Pathfinder::updateCurrent(float dt) noexcept
{
    // Update logic for Pathfinder can be implemented here
}

void Pathfinder::draw(RenderStates states) const noexcept
{
    mSprite.draw(states);
}

Textures::ID Pathfinder::getTextureID() const noexcept {
    switch (mType) {
        case Type::ALLY: {
            return Textures::ID::ASTRONAUT;
        }
        case Type::ENEMY: {
            return Textures::ID::ASTRONAUT;
        }
        default: {
            return Textures::ID::ASTRONAUT;
        }
    }
}
