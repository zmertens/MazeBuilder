#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "ResourceIdentifiers.hpp"
#include "SceneNode.hpp"

#include <box2d/box2d.h>

class CommandQueue;

class Entity : public SceneNode
{
public:
    Entity() noexcept;
    ~Entity() noexcept override;

    void accelerate(b2Vec2 velocityChange) noexcept;
    void accelerate(float vx, float vy) noexcept;

    void setVelocity(b2Vec2 velocity);
    void setVelocity(float vx, float vy);
    [[nodiscard]] b2Vec2 getVelocity() const;

    // Physics body lifecycle (C API)
    void createBody(b2WorldId worldId, const b2BodyDef* bodyDef) noexcept;
    void destroyBody(b2WorldId worldId) noexcept;

    [[nodiscard]] b2BodyId getBodyId() const noexcept { return mBodyId; }

    // Contact callbacks (override in subclasses as needed)
    virtual void onBeginContact(Entity* other) noexcept;
    virtual void onEndContact(Entity* other) noexcept;
    virtual void onPostSolve(Entity* other, float impulse) noexcept;

protected:

    void updateCurrent(float dt, CommandQueue&) noexcept override;

private:
    [[nodiscard]] virtual Textures::ID getTextureID() const noexcept = 0;

    b2Vec2 mVelocity;
    b2BodyId mBodyId;
};

#endif // ENTITY_HPP
