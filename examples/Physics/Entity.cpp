#include "Entity.hpp"

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

void Entity::updateCurrent(float dt)
{
    move(mVelocity * dt);
}
