#include "Pathfinder.hpp"

#include "ResourceManager.hpp"
#include "Texture.hpp"

Pathfinder::Pathfinder(Type type, const TextureManager& textures)
    : mType(type)
    , mSprite(textures.get(toTextureID(type)))
{
}

Pathfinder::Type Pathfinder::getCategory() const noexcept
{
    return mType;
}

void Pathfinder::updateCurrent(float dt) noexcept
{
    // Update logic for Pathfinder can be implemented here
}

void Pathfinder::draw(RenderStates states) const noexcept
{
    mSprite.draw(states);
}

Textures::ID Pathfinder::toTextureID(Type type) const noexcept {
    switch (type) {
        case Type::ALLY: {
            return Textures::ID::AVATAR;
        }
        case Type::ENEMY: {
            return Textures::ID::AVATAR;
        }
        default: {
            return Textures::ID::AVATAR;
        }
    }
}
