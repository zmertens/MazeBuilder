#include "World.hpp"

#include "RenderStates.hpp"
#include "SDLHelper.hpp"
#include "Texture.hpp"

#include "JsonUtils.hpp"

#include <SDL3/SDL.h>

#include <MazeBuilder/singleton_base.h>

#include <string>

World::World()
    : m_worldView()
    , m_textures()
    , m_sceneGraph()
    , m_sceneLayers()
    , m_forceDueToGravity{ -9.8f }
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = { 0.0f, m_forceDueToGravity };
    m_worldId = b2CreateWorld(&worldDef);

    loadTextures();
    buildScene();
}

World::~World() {
    if (isValid()) {
        destroyWorld();
    }
}

void World::update(float dt) {
    m_sceneGraph.update(dt);
}

void World::draw() const noexcept {
    RenderStates states;
    // In a real application, you would get the view from your camera
    // For now, we'll use a default RenderStates
    m_sceneGraph.draw(states);
}

b2WorldId World::getWorldId() const {
    return m_worldId;
}

/// @brief 
/// @param timeStep 
/// @param subSteps 6
void World::step(float timeStep, std::int32_t subSteps) {
    b2World_Step(m_worldId, timeStep, subSteps);
}

b2BodyId World::createBody(const b2BodyDef* bodyDef) {
    return b2CreateBody(m_worldId, bodyDef);
}

void World::destroyBody(b2BodyId bodyId) {
    b2DestroyBody(bodyId);
}

b2ShapeId World::createPolygonShape(b2BodyId bodyId, const b2ShapeDef* shapeDef, const b2Polygon* polygon) {
    return b2CreatePolygonShape(bodyId, shapeDef, polygon);
}

b2ShapeId World::createCircleShape(b2BodyId bodyId, const b2ShapeDef* shapeDef, const b2Circle* circle) {
    return b2CreateCircleShape(bodyId, shapeDef, circle);
}

b2ShapeId World::createSegmentShape(b2BodyId bodyId, const b2ShapeDef* shapeDef, const b2Segment* segment) {
    return b2CreateSegmentShape(bodyId, shapeDef, segment);
}

bool World::isValid() const {
    return b2World_IsValid(m_worldId);
}

b2ContactEvents World::getContactEvents() const {
    return b2World_GetContactEvents(m_worldId);
}

void World::destroyWorld() {
    if (b2World_IsValid(m_worldId))
    {
        b2DestroyWorld(m_worldId);
        m_worldId = b2_nullWorldId;
    }
}

void World::loadTextures() {

    using std::string;
    using std::unordered_map;

    // Now load resources after SDL is initialized
    JsonUtils jsonUtils{};
    unordered_map<string, string> resources{};
    TextureManager splashTextureManager;
    try {
        // Load resource configuration
        jsonUtils.loadConfiguration("resources/physics.json", ref(resources));
        SDL_Log(jsonUtils.getValue("splash_image", resources).c_str());
        splashTextureManager.load(Textures::ID::SPLASH_SCREEN, "resources/" + jsonUtils.getValue("splash_image", resources));
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
    for (std::size_t i = 0; i < static_cast<std::size_t>(Layer::LAYER_COUNT); ++i) {
        SceneNode::Ptr layer(new SceneNode());
        m_sceneLayers[i] = layer.get();
        m_sceneGraph.attachChild(std::move(layer));
    }
}
