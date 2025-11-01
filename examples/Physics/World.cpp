#include "World.hpp"

#include "Ball.hpp"
#include "JsonUtils.hpp"
#include "Pathfinder.hpp"
#include "RenderStates.hpp"
#include "RenderWindow.hpp"
#include "SDLHelper.hpp"
#include "SpriteNode.hpp"
#include "Texture.hpp"
#include "Wall.hpp"

#include <box2d/box2d.h>

#include <SDL3/SDL.h>

#include <MazeBuilder/singleton_base.h>

#include <string>

World::World(RenderWindow& window)
    : mWindow{ window }
    , mWorldView{ /* window.getView() */ }
    , mTextures{}
    , mSceneGraph{}
    , mSceneLayers{}
    , mWorldId{ b2_nullWorldId }
    , mCommandQueue{}
    , mPlayerPathfinder{ nullptr }
{
}

void World::init() noexcept {

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = { 0.0f, FORCE_DUE_TO_GRAVITY };
    mWorldId = b2CreateWorld(&worldDef);

    mPlayerPathfinder = nullptr;

    loadTextures();
    buildScene();
}

void World::update(float dt) {

    mWindow.setView(mWorldView);

    mSceneGraph.update(dt);
}

void World::draw() const noexcept {
    
    mWindow.draw(mSceneGraph);
}

CommandQueue& World::getCommandQueue() noexcept {

    return mCommandQueue;
}

void World::destroyWorld() {
    if (b2World_IsValid(mWorldId))
    {
        b2DestroyWorld(mWorldId);
        mWorldId = b2_nullWorldId;
    }
}

void World::loadTextures() {

    using std::string;
    using std::unordered_map;

    // Now load resources after SDL is initialized
    JsonUtils jsonUtils{};
    unordered_map<string, string> resources{};
    try {
        // Load resource configuration
        jsonUtils.loadConfiguration("resources/physics.json", ref(resources));
        SDL_Log(jsonUtils.getValue("splash_image", resources).c_str());
        auto splashImagePath = "resources/" + jsonUtils.getValue("splash_image", resources);
        SDL_Log("DEBUG: Loading splash screen from: %s", splashImagePath.c_str());
        mTextures.load(Textures::ID::SPLASH_SCREEN, splashImagePath);
        
        auto avatarValue = jsonUtils.getValue("avatar", resources);
        SDL_Log("DEBUG: Avatar value from JSON: '%s'", avatarValue.c_str());
        // Temporary fix - hardcode the avatar path since JSON parsing seems to have issues
        string avatarImagePath = "resources/character_beige_front.png";
        SDL_Log("DEBUG: Loading avatar from: %s", avatarImagePath.c_str());
        mTextures.load(Textures::ID::AVATAR, avatarImagePath);
    } catch (const std::exception& e) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load textures: %s", e.what());
        return;
    }

    SDL_Log("Successfully loaded all game resources");

    //sf::SoundBuffer generateSoundBuffer;

    // Load and set window icon from resources
    auto iconPath = "resources/" + jsonUtils.getValue("window_icon_path", resources);

    if (!iconPath.empty()) {
        SDL_Surface* icon = SDL_LoadBMP(iconPath.c_str());
        if (auto* sdlHelper = mazes::singleton_base<SDLHelper>::instance().get(); sdlHelper != nullptr && icon) {

            SDL_SetWindowIcon(sdlHelper->window, icon);
            SDL_DestroySurface(icon);
            SDL_Log("Successfully loaded window icon: %s", iconPath.c_str());
        } else {

            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load icon: %s - %s", iconPath.c_str(), SDL_GetError());
        }
    } else {
        SDL_Log("No window icon specified in configuration");
    }
}

void World::buildScene() {

    using std::cref;
    using std::make_unique;
    using std::move;
    using std::size_t;
    using std::unique_ptr;

    for (size_t i = 0; i < static_cast<size_t>(Layer::LAYER_COUNT); ++i) {
        SceneNode::Ptr layer = make_unique<SceneNode>();
        mSceneLayers[i] = layer.get();
        mSceneGraph.attachChild(move(layer));
    }

    auto& texture1 = mTextures.get(Textures::ID::SPLASH_SCREEN);
    SDL_Rect fullScreenRect = { 0, 0, texture1.getWidth(), texture1.getHeight() };
    auto backgroundSprite = make_unique<SpriteNode>(texture1, fullScreenRect);
    backgroundSprite->setPosition(0.0f, 0.0f);
    SceneNode::Ptr spriteNode = move(backgroundSprite);
    mSceneLayers[static_cast<std::size_t>(Layer::BACKGROUND)]->attachChild(move(spriteNode));

    auto leader = make_unique<Pathfinder>(Pathfinder::Type::ALLY, cref(mTextures));
    mPlayerPathfinder = leader.get();
    mPlayerPathfinder->setPosition(0.f, 0.f);
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(leader));

    // Create and add entities directly to scene graph - positioned to be visible
    auto ballNormal = make_unique<Ball>(Ball::Type::NORMAL, mTextures);
    auto* ballNormalPtr = ballNormal.get();
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(ballNormal));
    ballNormalPtr->setPosition(100.0f, 550.0f);  // Below splash screen
    
    auto ballHeavy = make_unique<Ball>(Ball::Type::HEAVY, mTextures);
    auto* ballHeavyPtr = ballHeavy.get();
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(ballHeavy));
    ballHeavyPtr->setPosition(250.0f, 550.0f);  // Below splash screen
    
    auto ballLight = make_unique<Ball>(Ball::Type::LIGHT, mTextures);
    auto* ballLightPtr = ballLight.get();
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(ballLight));
    ballLightPtr->setPosition(400.0f, 550.0f);  // Below splash screen
    
    auto ballExplosive = make_unique<Ball>(Ball::Type::EXPLOSIVE, mTextures);
    auto* ballExplosivePtr = ballExplosive.get();
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(ballExplosive));
    ballExplosivePtr->setPosition(550.0f, 550.0f);  // Below splash screen
    
    auto wallHorizontal = make_unique<Wall>(Wall::Orientation::HORIZONTAL, mTextures);
    auto* wallHorizontalPtr = wallHorizontal.get();
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(wallHorizontal));
    wallHorizontalPtr->setPosition(100.0f, 700.0f);  // Below balls
    
    auto wallVertical = make_unique<Wall>(Wall::Orientation::VERTICAL, mTextures);
    auto* wallVerticalPtr = wallVertical.get();
    mSceneLayers[static_cast<size_t>(Layer::FOREGROUND)]->attachChild(move(wallVertical));
    wallVerticalPtr->setPosition(250.0f, 700.0f);  // Below balls

    SDL_Log("World::buildScene - Scene built successfully");
}
