#include "Pathfinder.hpp"

#include "ResourceManager.hpp"
#include "Texture.hpp"

Pathfinder::Pathfinder(Type type, const TextureManager& textures)
    : mType(type)
    , mSprite(textures.get(getTextureID()))
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

Textures::ID Pathfinder::getTextureID() const noexcept {
    switch (mType) {
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
