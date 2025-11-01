#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "ResourceIdentifiers.hpp"
#include "SceneNode.hpp"

#include <box2d/math_functions.h>

class Entity : public SceneNode {
public:
    void setVelocity(b2Vec2 velocity);
    void setVelocity(float vx, float vy);
    b2Vec2 getVelocity() const;


private:

    virtual Textures::ID getTextureID() const noexcept = 0;

    virtual void updateCurrent(float dt) noexcept override;

private:
    b2Vec2 mVelocity;
};

#endif // ENTITY_HPP
