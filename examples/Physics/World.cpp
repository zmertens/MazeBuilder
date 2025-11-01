#include "World.hpp"

#include "Pathfinder.hpp"
#include "RenderStates.hpp"
#include "SDLHelper.hpp"
#include "SpriteNode.hpp"
#include "Texture.hpp"

#include "JsonUtils.hpp"

#include <SDL3/SDL.h>

#include <MazeBuilder/singleton_base.h>

#include <string>

void World::init() noexcept {

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = { 0.0f, FORCE_DUE_TO_GRAVITY };
    mWorldId = b2CreateWorld(&worldDef);

    mPlayerPathfinder = nullptr;

    loadTextures();
    buildScene();
}

void World::update(float dt) {
    mSceneGraph.update(dt);
}

void World::draw() const noexcept {
    RenderStates states;
    // In a real application, you would get the view from your camera
    // For now, we'll use a default RenderStates
    mSceneGraph.draw(states);
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
        mTextures.load(Textures::ID::SPLASH_SCREEN, "resources/" + jsonUtils.getValue("splash_image", resources));
    } catch (const std::exception& e) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load splash screen texture: %s", e.what());
        return;
    }

    SDL_Log("Successfully loaded all game resources");

    //sf::SoundBuffer generateSoundBuffer;

    // Load and set window icon from resources
    auto iconPath = "resources/" + jsonUtils.getValue("window_icon_path", resources);

    if (!iconPath.empty()) {
        SDL_Surface* icon = SDL_LoadBMP(iconPath.c_str());
        if (icon) {
            auto* sdlHelper = mazes::singleton_base<SDLHelper>::instance().get();
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
    SDL_Log("World::buildScene - Adding background sprite to scene");
    mSceneLayers[static_cast<std::size_t>(Layer::BACKGROUND)]->attachChild(move(spriteNode));

    auto leader = make_unique<Pathfinder>(Pathfinder::Type::ALLY, cref(mTextures));
    mPlayerPathfinder = leader.get();
    mPlayerPathfinder->setPosition(0.f, 0.f);
    mSceneLayers[static_cast<std::size_t>(Layer::FOREGROUND)]->attachChild(move(leader));
}
