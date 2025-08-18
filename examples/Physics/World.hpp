#ifndef WORLD_HPP
#define WORLD_HPP

#include <box2d/box2d.h>
#include <cstdint>

class World {
public:
    // Constructor that accepts gravity force
    explicit World(float forceDueToGravity);
    
    // Destructor
    ~World();
    
    // Get the Box2D world ID
    b2WorldId getWorldId() const;
    
    // Step the physics simulation forward
    void step(float timeStep, int32_t velocityIterations = 6, int32_t positionIterations = 2);
    
    // Create a body in the world
    b2BodyId createBody(const b2BodyDef* bodyDef);
    
    // Destroy a body in the world
    void destroyBody(b2BodyId bodyId);
    
    // Create a polygon shape for a body
    b2ShapeId createPolygonShape(b2BodyId bodyId, const b2ShapeDef* shapeDef, const b2Polygon* polygon);
    
    // Create a circle shape for a body
    b2ShapeId createCircleShape(b2BodyId bodyId, const b2ShapeDef* shapeDef, const b2Circle* circle);
    
    // Create a segment (line) shape for a body
    b2ShapeId createSegmentShape(b2BodyId bodyId, const b2ShapeDef* shapeDef, const b2Segment* segment);
    
    // Check if the world exists
    bool isValid() const;
    
    // Get contact events from the world
    b2ContactEvents getContactEvents() const;
    
    // Destroy the world
    void destroyWorld();
    
private:
    b2WorldId m_worldId = b2_nullWorldId;
};

#endif // WORLD_HPP
