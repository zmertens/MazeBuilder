#include "World.hpp"

#include "Ball.hpp"
#include "JsonUtils.hpp"
#include "Pathfinder.hpp"
#include "RenderWindow.hpp"
#include "SDLHelper.hpp"
#include "SpriteNode.hpp"
#include "Texture.hpp"
#include "Wall.hpp"

#include <box2d/box2d.h>

#include <SDL3/SDL.h>

#include <MazeBuilder/singleton_base.h>

#include <string>

World::World(RenderWindow& window, TextureManager& textures)
    : mWindow{window}
      , mWorldView{
          /* window.getView() */
      }
      , mTextures{textures}
      , mSceneGraph{}
      , mSceneLayers{}
      , mWorldId{b2_nullWorldId}
      , mCommandQueue{}
      , mPlayerPathfinder{nullptr}
{
}

void World::init() noexcept
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, FORCE_DUE_TO_GRAVITY};
    mWorldId = b2CreateWorld(&worldDef);

    mPlayerPathfinder = nullptr;

    buildScene();
}

void World::update(float dt)
{
    mWindow.setView(mWorldView);

    mSceneGraph.update(dt);
}

void World::draw() const noexcept
{
    mWindow.draw(mSceneGraph);
}

CommandQueue& World::getCommandQueue() noexcept
{
    return mCommandQueue;
}

void World::destroyWorld()
{
    if (b2World_IsValid(mWorldId))
    {
        b2DestroyWorld(mWorldId);
        mWorldId = b2_nullWorldId;
    }
}

void World::buildScene()
{
    using std::cref;
    using std::make_unique;
    using std::move;
    using std::size_t;
    using std::unique_ptr;

    for (size_t i = 0; i < static_cast<size_t>(Layer::LAYER_COUNT); ++i)
    {
        SceneNode::Ptr layer = make_unique<SceneNode>();
        mSceneLayers[i] = layer.get();
        mSceneGraph.attachChild(move(layer));
    }

    auto leader = make_unique<Pathfinder>(Pathfinder::Type::ALLY, cref(mTextures));
    mPlayerPathfinder = leader.get();
    mPlayerPathfinder->setPosition(0.f, 0.f);
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(leader));

    // Create and add entities directly to scene graph - positioned to be visible
    auto ballNormal = make_unique<Ball>(Ball::Type::NORMAL, mTextures);
    auto* ballNormalPtr = ballNormal.get();
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(ballNormal));
    ballNormalPtr->setPosition(100.0f, 550.0f); // Below splash screen

    auto ballHeavy = make_unique<Ball>(Ball::Type::HEAVY, mTextures);
    auto* ballHeavyPtr = ballHeavy.get();
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(ballHeavy));
    ballHeavyPtr->setPosition(250.0f, 550.0f); // Below splash screen

    auto ballLight = make_unique<Ball>(Ball::Type::LIGHT, mTextures);
    auto* ballLightPtr = ballLight.get();
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(ballLight));
    ballLightPtr->setPosition(400.0f, 550.0f); // Below splash screen

    auto ballExplosive = make_unique<Ball>(Ball::Type::EXPLOSIVE, mTextures);
    auto* ballExplosivePtr = ballExplosive.get();
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(ballExplosive));
    ballExplosivePtr->setPosition(550.0f, 550.0f); // Below splash screen

    auto wallHorizontal = make_unique<Wall>(Wall::Orientation::HORIZONTAL, mTextures);
    auto* wallHorizontalPtr = wallHorizontal.get();
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(wallHorizontal));
    wallHorizontalPtr->setPosition(100.0f, 700.0f); // Below balls

    auto wallVertical = make_unique<Wall>(Wall::Orientation::VERTICAL, mTextures);
    auto* wallVerticalPtr = wallVertical.get();
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(wallVertical));
    wallVerticalPtr->setPosition(250.0f, 700.0f); // Below balls

    // maze
    // set the texture that was procedurally generated from MazeBuilder in PhysicsGame
    auto& mazeTexture = mTextures.get(Textures::ID::SPLASH_SCREEN);
    SDL_Rect mazeRect = {0, 0, mazeTexture.getWidth(), mazeTexture.getHeight()};
    auto mazeSprite = make_unique<SpriteNode>(mazeTexture, mazeRect);
    mazeSprite->setPosition(0.0f, 0.0f);
    SceneNode::Ptr mazeNode = move(mazeSprite);
    mSceneLayers[static_cast<std::size_t>(Layer::BACKGROUND)]->attachChild(move(mazeNode));

    SDL_Log("World::buildScene - Scene built successfully");
}
