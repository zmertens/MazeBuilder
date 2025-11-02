#include "Ball.hpp"

#include <utility>

#include <SDL3/SDL.h>

#include <box2d/box2d.h>

#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"

Ball::Ball(Type type, const TextureManager& textureManager)
    : mType{type}, mSprite{textureManager.get(getTextureID())} {
}

// // Create a dynamic body for the ball
// b2BodyDef bodyDef = b2DefaultBodyDef();
// bodyDef.type = b2_dynamicBody;
// bodyDef.position = { std::get<0>(coords), std::get<1>(coords) };

// bodyDef.linearVelocity = {
//     static_cast<float>(static_cast<int>(std::get<1>(coords)) % 100) - 50.f / 30.0f,
//     static_cast<float>(static_cast<int>(std::get<0>(coords)) % 100) - 50.f / 30.0f
// };

// // Reduced damping for more movement
// bodyDef.linearDamping = 0.2f;
// bodyDef.angularDamping = 0.4f;
// // Enable continuous collision detection
// bodyDef.isBullet = true;
// bodyDef.userData = reinterpret_cast<void*>(this);

// b2BodyId ballBodyId = b2CreateBody(worldId, &bodyDef);

// // Explicitly set the body to be awake
// b2Body_SetAwake(ballBodyId, true);

// // Create a circle shape for the ball
// b2ShapeDef shapeDef = b2DefaultShapeDef();
// // Heavier balls for better collision impacts
// shapeDef.density = 1.5f;
// // Lower resistance for smoother rolling
// shapeDef.material.rollingResistance = 0.1f;
// shapeDef.material.friction = 0.2f;
// // Higher restitution for more bounce
// shapeDef.material.restitution = 0.8f;

// // In Box2D 3.1.0, the circle is defined separately from the shape def
// b2Circle circle = { {0.f, 0.f}, r };

// b2ShapeId ballShapeId = b2CreateCircleShape(ballBodyId, &shapeDef, &circle);

// bodyId = ballBodyId;
// shapeId = ballShapeId;
// isActive = true;

void Ball::updateCurrent(float dt) noexcept {

}


// Draw the ball
void Ball::drawCurrent(RenderStates states) const noexcept {
    static int drawCount = 0;
    if (drawCount < 5) {
        SDL_Log("Ball::drawCurrent - Drawing ball at pos(%.1f,%.1f)", 
                states.transform.p.x, states.transform.p.y);
        drawCount++;
    }
    mSprite.draw(states);
}

Textures::ID Ball::getTextureID() const noexcept {

    switch (mType) {
        case Type::NORMAL: {
            return Textures::ID::BALL_NORMAL;
        }
        case Type::HEAVY: {
            return Textures::ID::BALL_NORMAL;
        }
        case Type::LIGHT: {
            return Textures::ID::BALL_NORMAL;
        }
        case Type::EXPLOSIVE: {
            return Textures::ID::BALL_NORMAL;
        }
        default: {
            return Textures::ID::BALL_NORMAL;
        }
    }
}