#include "World.hpp"
#include <SDL3/SDL.h>

World::World(float forceDueToGravity) {
    // Set length units per meter as recommended in Box2D FAQ
    // This gives us a better scale for the simulation
    // Box2D works best with moving objects between 0.1 and 10 meters in size
    float lengthUnitsPerMeter = 1.0f;
    b2SetLengthUnitsPerMeter(lengthUnitsPerMeter);

    b2WorldDef worldDef = b2DefaultWorldDef();

    // Use specified gravity for gameplay
    worldDef.gravity.y = forceDueToGravity;
    
    // Create physics world
    m_worldId = b2CreateWorld(&worldDef);
    
    SDL_Log("Box2D physics world initialized with gravity: %f", worldDef.gravity.y);
}

World::~World() {
    destroyWorld();
}

void World::destroyWorld() {
    if (B2_IS_NON_NULL(m_worldId)) {
        b2DestroyWorld(m_worldId);
        m_worldId = b2_nullWorldId;
    }
}

b2WorldId World::getWorldId() const {
    return m_worldId;
}

void World::step(float timeStep, int32_t velocityIterations, int32_t positionIterations) {
    if (B2_IS_NON_NULL(m_worldId)) {
        b2World_Step(m_worldId, timeStep, velocityIterations);
    }
}

b2BodyId World::createBody(const b2BodyDef* bodyDef) {
    if (B2_IS_NON_NULL(m_worldId)) {
        return b2CreateBody(m_worldId, bodyDef);
    }
    return b2_nullBodyId;
}

void World::destroyBody(b2BodyId bodyId) {
    if (B2_IS_NON_NULL(m_worldId) && B2_IS_NON_NULL(bodyId)) {
        b2DestroyBody(bodyId);
    }
}

b2ShapeId World::createPolygonShape(b2BodyId bodyId, const b2ShapeDef* shapeDef, const b2Polygon* polygon) {
    if (B2_IS_NON_NULL(m_worldId) && B2_IS_NON_NULL(bodyId)) {
        return b2CreatePolygonShape(bodyId, shapeDef, polygon);
    }
    return b2_nullShapeId;
}

b2ShapeId World::createCircleShape(b2BodyId bodyId, const b2ShapeDef* shapeDef, const b2Circle* circle) {
    if (B2_IS_NON_NULL(m_worldId) && B2_IS_NON_NULL(bodyId)) {
        return b2CreateCircleShape(bodyId, shapeDef, circle);
    }
    return b2_nullShapeId;
}

b2ShapeId World::createSegmentShape(b2BodyId bodyId, const b2ShapeDef* shapeDef, const b2Segment* segment) {
    if (B2_IS_NON_NULL(m_worldId) && B2_IS_NON_NULL(bodyId)) {
        return b2CreateSegmentShape(bodyId, shapeDef, segment);
    }
    return b2_nullShapeId;
}

bool World::isValid() const {
    return B2_IS_NON_NULL(m_worldId);
}

b2ContactEvents World::getContactEvents() const {
    if (B2_IS_NON_NULL(m_worldId)) {
        return b2World_GetContactEvents(m_worldId);
    }
    return {};
}