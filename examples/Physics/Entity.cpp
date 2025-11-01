#include "Entity.hpp"

void Entity::accelerate(b2Vec2 velocityChange) noexcept {
    mVelocity += velocityChange;
}

void Entity::accelerate(float vx, float vy) noexcept {
    mVelocity.x += vx;
    mVelocity.y += vy;
}

void Entity::setVelocity(b2Vec2 velocity)
{
    mVelocity = velocity;
}

void Entity::setVelocity(float vx, float vy)
{
    mVelocity.x = vx;
    mVelocity.y = vy;
}

b2Vec2 Entity::getVelocity() const
{
    return mVelocity;
}

void Entity::updateCurrent(float dt) noexcept
{
    move(mVelocity * dt);
}
