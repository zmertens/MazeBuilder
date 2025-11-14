#ifndef WORLD_HPP
#define WORLD_HPP

#include <box2d/box2d.h>

#include <array>
#include <vector>

#include "CommandQueue.hpp"
#include "RenderWindow.hpp"
#include "ResourceIdentifiers.hpp"
#include "SceneNode.hpp"
#include "View.hpp"

class Pathfinder;
class RenderWindow;

class World final
{
public:
    explicit World(RenderWindow& window, FontManager& fonts, TextureManager& textures);

    void init() noexcept;

    // Update the world (update physics and entities)
    void update(float dt);

    // Draw the world (render entities) - pass renderer for drawing
    void draw() const noexcept;

    CommandQueue& getCommandQueue() noexcept;

    // Destroy the world
    void destroyWorld();

private:
    // Build the scene (initialize scene graph and layers)
    void buildScene();

    enum class Layer
    {
        BACKGROUND = 0,
        FOREGROUND = 1,
        LAYER_COUNT = 2
    };

    static constexpr auto FORCE_DUE_TO_GRAVITY = -9.8f;

    RenderWindow& mWindow;
    View mWorldView;
    FontManager& mFonts;
    TextureManager& mTextures;

    SceneNode mSceneGraph;
    std::array<SceneNode*, static_cast<std::size_t>(Layer::LAYER_COUNT)> mSceneLayers;

    b2WorldId mWorldId;

    CommandQueue mCommandQueue;

    Pathfinder* mPlayerPathfinder;
};

#endif // WORLD_HPP
