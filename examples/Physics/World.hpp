#ifndef WORLD_HPP
#define WORLD_HPP

#include <box2d/id.h>

#include <array>
#include <cstdint>
#include <vector>

#include "Command.hpp"
#include "CommandQueue.hpp"
#include "RenderWindow.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "SceneNode.hpp"
#include "View.hpp"

class Pathfinder;
class RenderWindow;

class World final {
public:

    explicit World(RenderWindow& window);

    void init() noexcept;

    void destroy() noexcept;
    
    // Update the world (update physics and entities)
    void update(float dt);
    
    // Draw the world (render entities) - pass renderer for drawing
    void draw() const noexcept;
    
    CommandQueue& getCommandQueue() noexcept;

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

    static constexpr auto FORCE_DUE_TO_GRAVITY = -9.8f;

    RenderWindow& mWindow;
    View mWorldView;
    TextureManager mTextures;

    SceneNode mSceneGraph;
    std::array<SceneNode*, static_cast<std::size_t>(Layer::LAYER_COUNT)> mSceneLayers;

    b2WorldId mWorldId;

    CommandQueue mCommandQueue;

    Pathfinder* mPlayerPathfinder;
};

#endif // WORLD_HPP
