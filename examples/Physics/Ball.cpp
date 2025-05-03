#include "Ball.hpp"

#include <utility>

Ball::Ball(std::tuple<float, float, float> coords, float r, const b2WorldId worldId)
    : coords{ coords }, radius{ r }
    , bodyId{ b2_nullBodyId }
    , shapeId{ b2_nullShapeId }
    , isActive{ false }
    , isDragging{ false }
    , isExploding{ false }
    , explosionTimer{ 0.f } {

    // Create a dynamic body for the ball
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = { std::get<0>(coords), std::get<1>(coords) };

    bodyDef.linearVelocity = {
        static_cast<float>(static_cast<int>(std::get<1>(coords)) % 100) - 50.f / 30.0f,
        static_cast<float>(static_cast<int>(std::get<0>(coords)) % 100) - 50.f / 30.0f
    };

    // Reduced damping for more movement
    bodyDef.linearDamping = 0.2f;
    bodyDef.angularDamping = 0.4f;
    // Enable continuous collision detection
    bodyDef.isBullet = true;
    bodyDef.userData = reinterpret_cast<void*>(this);

    b2BodyId ballBodyId = b2CreateBody(worldId, &bodyDef);

    // Explicitly set the body to be awake
    b2Body_SetAwake(ballBodyId, true);

    // Create a circle shape for the ball
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    // Heavier balls for better collision impacts
    shapeDef.density = 1.5f;
    // Lower resistance for smoother rolling
    shapeDef.material.rollingResistance = 0.1f;
    shapeDef.material.friction = 0.2f;
    // Higher restitution for more bounce
    shapeDef.material.restitution = 0.8f;

    // In Box2D 3.1.0, the circle is defined separately from the shape def
    b2Circle circle = { {0.f, 0.f}, r };

    b2ShapeId ballShapeId = b2CreateCircleShape(ballBodyId, &shapeDef, &circle);

    bodyId = ballBodyId;
    shapeId = ballShapeId;
    isActive = true;
}

// Getters
b2BodyId Ball::getBodyId() const { return bodyId; }
b2ShapeId Ball::getShapeId() const { return shapeId; }
bool Ball::getIsActive() const { return isActive; }
bool Ball::getIsDragging() const { return isDragging; }
bool Ball::getIsExploding() const { return isExploding; }
float Ball::getExplosionTimer() const { return explosionTimer; }
std::tuple<float, float, float> Ball::getCoords() const { return coords; }
float Ball::getRadius() const { return radius; }

// Setters
void Ball::setBodyId(const b2BodyId& id) { bodyId = id; }
void Ball::setShapeId(const b2ShapeId& id) { shapeId = id; }
void Ball::setIsActive(bool active) { isActive = active; }
void Ball::setIsDragging(bool dragging) { isDragging = dragging; }
void Ball::setIsExploding(bool exploding) { isExploding = exploding; }
void Ball::setExplosionTimer(float timer) { explosionTimer = timer; }
void Ball::setCoords(const std::tuple<float, float, float>& newCoords) { coords = newCoords; }
void Ball::setRadius(float newRadius) { radius = newRadius; }
