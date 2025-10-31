#ifndef WORLD_HPP
#define WORLD_HPP

#include <box2d/box2d.h>

#include <cstdint>

#include "SceneNode.hpp"
#include "View.hpp"
#include "ResourceManager.hpp"
#include "ResourceIdentifiers.hpp"

#include <array>

struct SDL_Renderer;

class World {
public:

    explicit World();
    
    // Destructor
    ~World();
    
    // Update the world (update physics and entities)
    void update(float dt);
    
    // Draw the world (render entities)
    void draw() const noexcept;
    
    // Get the Box2D world ID
    b2WorldId getWorldId() const;
    
    // Step the physics simulation forward
    void step(float timeStep, std::int32_t subSteps = 6);
    
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
    // Load textures for the world
    void loadTextures();
    
    // Build the scene (initialize scene graph and layers)
    void buildScene();

private:
    enum class Layer {
        BACKGROUND = 0,
        FOREGROUND = 1,
        LAYER_COUNT = 2
    };

private:

    View m_worldView;
    TextureManager m_textures;

    SceneNode m_sceneGraph;
    std::array<SceneNode*, static_cast<std::size_t>(Layer::LAYER_COUNT)> m_sceneLayers;

    b2WorldId m_worldId;
    float m_forceDueToGravity;
};

#endif // WORLD_HPP
