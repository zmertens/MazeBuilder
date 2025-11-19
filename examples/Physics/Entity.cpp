#include "Entity.hpp"

#include "CommandQueue.hpp"
#include "Physics.hpp"

#include <box2d/box2d.h>

#include <SDL3/SDL.h>

Entity::Entity() noexcept
    : mVelocity{0.f, 0.f}
    , mBodyId{b2_nullBodyId}
{
}

Entity::~Entity() noexcept = default;

void Entity::accelerate(b2Vec2 velocityChange) noexcept
{
    if (b2Body_IsValid(mBodyId))
    {
        b2Vec2 v = b2Body_GetLinearVelocity(mBodyId);
        v.x += velocityChange.x;
        v.y += velocityChange.y;
        b2Body_SetLinearVelocity(mBodyId, v);
    }
    else
    {
        mVelocity.x += velocityChange.x;
        mVelocity.y += velocityChange.y;
    }
}

void Entity::accelerate(float vx, float vy) noexcept
{
    accelerate({vx, vy});
}

void Entity::setVelocity(b2Vec2 velocity)
{
    if (b2Body_IsValid(mBodyId))
    {
        b2Body_SetLinearVelocity(mBodyId, velocity);
    }
    else
    {
        mVelocity = velocity;
    }
}

void Entity::setVelocity(float vx, float vy)
{
    setVelocity({vx, vy});
}

b2Vec2 Entity::getVelocity() const
{
    if (b2Body_IsValid(mBodyId))
    {
        return b2Body_GetLinearVelocity(mBodyId);
    }
    return mVelocity;
}

void Entity::createBody(b2WorldId worldId, const b2BodyDef* bodyDef) noexcept
{
    if (!b2World_IsValid(worldId) || b2Body_IsValid(mBodyId))
        return;

    mBodyId = b2CreateBody(worldId, bodyDef);
    if (b2Body_IsValid(mBodyId))
    {
        // store pointer to this entity in user data for contact callbacks
        b2Body_SetUserData(mBodyId, this);
    }
}

void Entity::destroyBody(b2WorldId worldId) noexcept
{
    if (!b2World_IsValid(worldId) || !b2Body_IsValid(mBodyId))
        return;

    b2DestroyBody(mBodyId);
    mBodyId = b2_nullBodyId;
}

void Entity::onBeginContact(Entity* /*other*/) noexcept
{
    // default: do nothing
}

void Entity::onEndContact(Entity* /*other*/) noexcept
{
    // default: do nothing
}

void Entity::onPostSolve(Entity* /*other*/, float /*impulse*/) noexcept
{
    // default: do nothing
}

void Entity::updateCurrent(float dt, [[maybe_unused]] CommandQueue&) noexcept
{
    if (b2Body_IsValid(mBodyId))
    {
        b2Vec2 pos = b2Body_GetPosition(mBodyId);
        b2Rot rot = b2Body_GetRotation(mBodyId);

        // Box2D stores meters; convert to pixels for the scene graph
        b2Vec2 pixelPos = physics::toPixelsVec(pos);
        setPosition(pixelPos);
        setRotation(rot);
    }
    else
    {
        // fallback to manual movement
        b2Vec2 offset = {mVelocity.x * dt, mVelocity.y * dt};
        move(offset);
    }
}

