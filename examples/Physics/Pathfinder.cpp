#include "Pathfinder.hpp"

void Pathfinder::setVelocity(b2Vec2 velocity)
{
    mVelocity = velocity;
}

void Pathfinder::setVelocity(float vx, float vy)
{
    mVelocity.x = vx;
    mVelocity.y = vy;
}

b2Vec2 Pathfinder::getVelocity() const
{
    return mVelocity;
}

void Pathfinder::updateCurrent(float dt)
{
    move(mVelocity * dt);
}
