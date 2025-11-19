#include "World.hpp"

#include "Ball.hpp"
#include "Entity.hpp"
#include "JsonUtils.hpp"
#include "ParallaxNode.hpp"
#include "Pathfinder.hpp"
#include "RenderWindow.hpp"
#include "ResourceManager.hpp"
#include "SpriteNode.hpp"
#include "Texture.hpp"
#include "Wall.hpp"
#include "MazeLayout.hpp"

#include "Physics.hpp"
#include "PhysicsContactListener.hpp"

#include <MazeBuilder/create.h>

#include <box2d/box2d.h>

#include <SDL3/SDL.h>

World::World(RenderWindow& window, FontManager& fonts, TextureManager& textures)
    : mWindow{window}
      , mWorldView{window.getView()}
      , mFonts{fonts}
      , mTextures{textures}
      , mSceneGraph{}
      , mSceneLayers{}
      , mWorldId{b2_nullWorldId}
      , mCommandQueue{}
      , mPlayerPathfinder{nullptr}
      , mIsPanning{false}
      , mLastMousePosition{0.f, 0.f}
{
}

void World::init() noexcept
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, FORCE_DUE_TO_GRAVITY};

    mWorldId = b2CreateWorld(&worldDef);

    mPostProcessingManager = std::make_unique<PostProcessingManager>();
    if (!mPostProcessingManager->initialize(mWindow.getRenderer(),
        static_cast<int>(mWindow.getView().getSize().x),
        static_cast<int>(mWindow.getView().getSize().y))) {
        SDL_Log("WARNING: Failed to initialize post-processing");
        mPostProcessingManager.reset(); // Continue without effects
    }

    // Optional: Configure effects
    if (mPostProcessingManager) {
        mPostProcessingManager->setBlurRadius(2);
        mPostProcessingManager->setBloomThreshold(0.75f);
        mPostProcessingManager->setBloomIntensity(1.2f);
    }

    mPlayerPathfinder = nullptr;

    buildScene();
}

void World::update(float dt)
{
    // Reset player velocity before processing commands (like SFML does)
    if (mPlayerPathfinder)
    {
        mPlayerPathfinder->setVelocity(0.f, 0.f);
        mWorldView.setCenter(mPlayerPathfinder->getPosition().x, mPlayerPathfinder->getPosition().y);
    }

    mWindow.setView(mWorldView);

    // Process commands from the queue BEFORE physics step (like SFML does)
    // This ensures player input forces are applied in the same frame
    while (!mCommandQueue.isEmpty())
    {
        Command command = mCommandQueue.pop();
        mSceneGraph.onCommand(command, dt);
    }

    // Step physics simulation (integrates forces applied by commands)
    if (b2World_IsValid(mWorldId))
    {
        b2World_Step(mWorldId, dt, 4);

#if defined(MAZE_DEBUG)
        static int stepCounter = 0;
        if (stepCounter++ % 60 == 0)
        {
            b2Counters counters = b2World_GetCounters(mWorldId);
            SDL_Log("Physics step #%d: bodies=%d, contacts=%d", stepCounter, counters.bodyCount, counters.contactCount);
        }
#endif

        // Poll contact events after stepping
        b2ContactEvents events = b2World_GetContactEvents(mWorldId);

        // Process begin contact events
        for (int i = 0; i < events.beginCount; ++i)
        {
            b2ContactBeginTouchEvent* beginEvent = events.beginEvents + i;
            b2BodyId bodyIdA = b2Shape_GetBody(beginEvent->shapeIdA);
            b2BodyId bodyIdB = b2Shape_GetBody(beginEvent->shapeIdB);

            if (b2Body_IsValid(bodyIdA) && b2Body_IsValid(bodyIdB))
            {
                void* userDataA = b2Body_GetUserData(bodyIdA);
                void* userDataB = b2Body_GetUserData(bodyIdB);

                auto* entityA = static_cast<Entity*>(userDataA);
                auto* entityB = static_cast<Entity*>(userDataB);

                if (entityA) entityA->onBeginContact(entityB);
                if (entityB) entityB->onBeginContact(entityA);
            }
        }

        // Process end contact events
        for (int i = 0; i < events.endCount; ++i)
        {
            b2ContactEndTouchEvent* endEvent = events.endEvents + i;
            b2BodyId bodyIdA = b2Shape_GetBody(endEvent->shapeIdA);
            b2BodyId bodyIdB = b2Shape_GetBody(endEvent->shapeIdB);

            if (b2Body_IsValid(bodyIdA) && b2Body_IsValid(bodyIdB))
            {
                void* userDataA = b2Body_GetUserData(bodyIdA);
                void* userDataB = b2Body_GetUserData(bodyIdB);

                Entity* entityA = static_cast<Entity*>(userDataA);
                Entity* entityB = static_cast<Entity*>(userDataB);

                if (entityA) entityA->onEndContact(entityB);
                if (entityB) entityB->onEndContact(entityA);
            }
        }
    }


    // Update scene graph (this calls Entity::updateCurrent which syncs transforms)
    mSceneGraph.update(dt, std::ref(mCommandQueue));
}

void World::draw() const noexcept
{
    if (mPostProcessingManager->isReady())
    {
        auto* renderer = mWindow.getRenderer();

        mPostProcessingManager->beginScene();

        mWindow.draw(mSceneGraph);

        mPostProcessingManager->endScene();

        mWindow.clear();

        mPostProcessingManager->present(renderer);
    }
}

CommandQueue& World::getCommandQueue() noexcept
{
    return mCommandQueue;
}

void World::handleEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_MOUSE_WHEEL:
        if (event.wheel.y > 0)
            mWorldView.zoom(1.1f);
        else if (event.wheel.y < 0)
            mWorldView.zoom(0.9f);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event.button.button == SDL_BUTTON_MIDDLE)
        {
            mIsPanning = true;
            mLastMousePosition = {static_cast<float>(event.button.x), static_cast<float>(event.button.y)};
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.button.button == SDL_BUTTON_MIDDLE)
        {
            mIsPanning = false;
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
        if (mIsPanning)
        {
            SDL_FPoint currentMousePosition = {static_cast<float>(event.motion.x), static_cast<float>(event.motion.y)};
            SDL_FPoint delta = {currentMousePosition.x - mLastMousePosition.x, currentMousePosition.y - mLastMousePosition.y};
            mLastMousePosition = currentMousePosition;

            if (SDL_GetModState() & SDL_KMOD_SHIFT)
            {
                mWorldView.rotate(delta.x);
            }
            else
            {
                mWorldView.move(-delta.x, -delta.y);
            }
        }
        break;
    }
}

void World::destroyWorld()
{
    if (b2World_IsValid(mWorldId))
    {
        b2DestroyWorld(mWorldId);
        mWorldId = b2_nullWorldId;
    }
}

void World::setPlayer(Player* player)
{
    if (mPlayerPathfinder)
    {
        // mPlayerPathfinder->setPosition(player->)
    }
}

void World::buildScene()
{
    using std::cref;
    using std::make_unique;
    using std::size_t;
    using std::unique_ptr;

    for (size_t i = 0; i < static_cast<size_t>(Layer::LAYER_COUNT); ++i)
    {
        auto layer = make_unique<SceneNode>();
        mSceneLayers[i] = layer.get();
        mSceneGraph.attachChild(std::move(layer));
    }

    // Create parallax background layers using LEVEL_ONE texture
    // Each layer scrolls at a different speed to create depth effect
    // Negative speeds scroll left (like in the raylib example)

    // Back layer - slowest scroll
    auto parallaxBack = make_unique<ParallaxNode>(mTextures.get(Textures::ID::CHARACTER_SPRITE_SHEET), -20.0f);
    parallaxBack->setPosition(0.f, 0.f);
    parallaxBack->setScale(2.0f);
    parallaxBack->setVerticalOffset(20.0f);
    mSceneLayers[static_cast<size_t>(Layer::PARALLAX_BACK)]->attachChild(std::move(parallaxBack));

    // Mid layer - medium scroll
    auto parallaxMid = make_unique<ParallaxNode>(mTextures.get(Textures::ID::LEVEL_TWO), -50.0f);
    parallaxMid->setPosition(0.f, 0.f);
    parallaxMid->setScale(2.0f);
    parallaxMid->setVerticalOffset(20.0f);
    mSceneLayers[static_cast<size_t>(Layer::PARALLAX_MID)]->attachChild(std::move(parallaxMid));

    // Fore layer - fastest scroll
    auto parallaxFore = make_unique<ParallaxNode>(mTextures.get(Textures::ID::LEVEL_ONE), -100.0f);
    parallaxFore->setPosition(0.f, 0.f);
    parallaxFore->setScale(2.0f);
    parallaxFore->setVerticalOffset(70.0f);
    mSceneLayers[static_cast<size_t>(Layer::PARALLAX_FORE)]->attachChild(std::move(parallaxFore));

    // // Static background with LEVEL_TWO
    // auto mazeNode = make_unique<SpriteNode>(mTextures.get(Textures::ID::LEVEL_TWO));
    // mazeNode->setPosition(0.f, 0.f);
    // mSceneLayers[static_cast<size_t>(Layer::BACKGROUND)]->attachChild(std::move(mazeNode));
    //
    // auto leader = make_unique<Pathfinder>(Pathfinder::Type::ALLY, cref(mTextures));
    // mPlayerPathfinder = leader.get();
    // mPlayerPathfinder->setPosition(0.f, 0.f);
    // mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(std::move(leader));
    //
    // // Create and add entities directly to scene graph - positioned to be visible
    // auto ballNormal = make_unique<Ball>(Ball::Type::NORMAL, mTextures);
    // auto* ballNormalPtr = ballNormal.get();
    // mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(std::move(ballNormal));
    // ballNormalPtr->setPosition(100.0f, 550.0f); // Below splash screen
    //
    // auto ballHeavy = make_unique<Ball>(Ball::Type::HEAVY, mTextures);
    // auto* ballHeavyPtr = ballHeavy.get();
    // mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(std::move(ballHeavy));
    // ballHeavyPtr->setPosition(250.0f, 550.0f); // Below splash screen
    //
    // auto ballLight = make_unique<Ball>(Ball::Type::LIGHT, mTextures);
    // auto* ballLightPtr = ballLight.get();
    // mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(std::move(ballLight));
    // ballLightPtr->setPosition(400.0f, 550.0f); // Below splash screen
    //
    // auto ballExplosive = make_unique<Ball>(Ball::Type::EXPLOSIVE, mTextures);
    // auto* ballExplosivePtr = ballExplosive.get();
    // mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(std::move(ballExplosive));
    // ballExplosivePtr->setPosition(550.0f, 550.0f); // Below splash screen
    //
    // auto wallHorizontal = make_unique<Wall>(Wall::Orientation::HORIZONTAL, mTextures);
    // auto* wallHorizontalPtr = wallHorizontal.get();
    // mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(std::move(wallHorizontal));
    // wallHorizontalPtr->setPosition(100.0f, 700.0f); // Below balls
    //
    // auto wallVertical = make_unique<Wall>(Wall::Orientation::VERTICAL, mTextures);
    // auto* wallVerticalPtr = wallVertical.get();
    // mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(std::move(wallVertical));
    // wallVerticalPtr->setPosition(250.0f, 700.0f); // Below balls
    //
    // // Create physics bodies for the created entities
    // if (b2World_IsValid(mWorldId))
    // {
    //     b2BodyDef backgroundBodyDef = b2DefaultBodyDef();
    //     backgroundBodyDef.type = b2_staticBody;
    //     backgroundBodyDef.position = {0, 0};
    //     b2BodyId backgroundBodyId = b2CreateBody(mWorldId, &backgroundBodyDef);
    //
    //     // Helper lambda to create a dynamic circular ball
    //     auto createBallBody = [&](Ball* b, Ball::Type type, float radiusPx, float density, float restitution, float friction, bool bullet)
    //     {
    //         b2BodyDef bodyDef = b2DefaultBodyDef();
    //         bodyDef.type = b2_dynamicBody;
    //         b2Vec2 posMeters = physics::toMetersVec(b->getPosition());
    //         bodyDef.position = posMeters;
    //         bodyDef.linearDamping = 0.2f;
    //         bodyDef.angularDamping = 0.4f;
    //         bodyDef.isBullet = bullet;
    //
    //         b->createBody(mWorldId, &bodyDef);
    //         b2BodyId bodyId = b->getBodyId();
    //         if (!b2Body_IsValid(bodyId))
    //         {
    //             SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create ball body!");
    //             return;
    //         }
    //
    //         b2ShapeDef shapeDef = b2DefaultShapeDef();
    //         shapeDef.density = density;
    //
    //         b2Circle circle = {{0.0f, 0.0f}, physics::toMeters(radiusPx)};
    //         b2ShapeId shapeId = b2CreateCircleShape(bodyId, &shapeDef, &circle);
    //
    //         // Set friction and restitution after creation
    //         b2Shape_SetFriction(shapeId, friction);
    //         b2Shape_SetRestitution(shapeId, restitution);
    //
    //         b2Body_SetAwake(bodyId, true);
    //     };
    //
    //     // Create the balls
    //     createBallBody(ballNormalPtr, Ball::Type::NORMAL, 16.0f, 1.0f, 0.6f, 0.3f, true);
    //     createBallBody(ballHeavyPtr, Ball::Type::HEAVY, 20.0f, 3.0f, 0.4f, 0.25f, true);
    //     createBallBody(ballLightPtr, Ball::Type::LIGHT, 14.0f, 0.6f, 0.7f, 0.2f, true);
    //     createBallBody(ballExplosivePtr, Ball::Type::EXPLOSIVE, 18.0f, 1.5f, 0.5f, 0.25f, true);
    //
    //     // Create walls as static boxes
    //     auto createWallBody = [&](Wall* w, float halfWidthPx, float halfHeightPx)
    //     {
    //         b2BodyDef bodyDef = b2DefaultBodyDef();
    //         bodyDef.type = b2_staticBody;
    //         bodyDef.position = physics::toMetersVec(w->getPosition());
    //
    //         w->createBody(mWorldId, &bodyDef);
    //         b2BodyId bodyId = w->getBodyId();
    //         if (!b2Body_IsValid(bodyId)) return;
    //
    //         b2ShapeDef shapeDef = b2DefaultShapeDef();
    //         shapeDef.density = 0.0f;
    //
    //         b2Polygon box = b2MakeBox(physics::toMeters(halfWidthPx), physics::toMeters(halfHeightPx));
    //         b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &box);
    //
    //         // Set friction and restitution after creation
    //         b2Shape_SetFriction(shapeId, 0.8f);
    //         b2Shape_SetRestitution(shapeId, 0.0f);
    //     };
    //
    //     // horizontal wall: wide and short
    //     createWallBody(wallHorizontalPtr, 120.0f, 10.0f);
    //     // vertical wall: tall and narrow
    //     createWallBody(wallVerticalPtr, 10.0f, 120.0f);
    //
    //     // Create pathfinder as a kinematic body for direct control
    //     if (mPlayerPathfinder)
    //     {
    //         b2BodyDef bodyDef = b2DefaultBodyDef();
    //         bodyDef.type = b2_dynamicBody;
    //         bodyDef.position = physics::toMetersVec(mPlayerPathfinder->getPosition());
    //         bodyDef.fixedRotation = true;
    //
    //         mPlayerPathfinder->createBody(mWorldId, &bodyDef);
    //         b2BodyId bodyId = mPlayerPathfinder->getBodyId();
    //         if (b2Body_IsValid(bodyId))
    //         {
    //             b2ShapeDef shapeDef = b2DefaultShapeDef();
    //             shapeDef.density = 1.0f;
    //
    //             // small box for the pathfinder
    //             b2Polygon box = b2MakeBox(physics::toMeters(16.0f), physics::toMeters(24.0f));
    //             b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &box);
    //
    //             // Set friction and restitution after creation
    //             b2Shape_SetFriction(shapeId, 0.4f);
    //             b2Shape_SetRestitution(shapeId, 0.1f);
    //
    //             b2Body_SetAwake(bodyId, true);
    //         }
    //     }
    // }
}
