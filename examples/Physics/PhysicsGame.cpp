//
// PhysicsGame class implementation
// Simple 2D physics simulation with bouncy balls that break walls
//
// Audio Handling reference from SDL_AUDIO_STREAM: SDL\test\testaudio.c
//
//

#include "PhysicsGame.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string_view>
#include <string>
#include <variant>
#include <vector>

#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include <SFML/Audio.hpp>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_opengl3.h>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/json_helper.h>

#include "AudioHelper.hpp"
#include "Ball.hpp"
#include "CoutThreadSafe.hpp"
#include "Drawable.hpp"
#include "OrthographicCamera.hpp"
#include "Physical.hpp"
#include "Renderer.hpp"
#include "ResourceManager.hpp"
#include "SDLHelper.hpp"
#include "State.hpp"
#include "Texture.hpp"
#include "Wall.hpp"
#include "WorkerConcurrent.hpp"
#include "World.hpp"

using Resources = ResourceManager::PhysicsResources;

struct PhysicsGame::PhysicsGameImpl {
    
    // Game-specific constants
    static constexpr float WALL_HIT_THRESHOLD = 4.0f; // Number of hits before a wall breaks
    static constexpr float WALL_WIDTH = 0.1f;

    static constexpr int MAX_BALLS = 10;
    
    // Maze rendering variables
    float cellSize = 0.0f;    // Size of a maze cell in pixels
    float offsetX = 0.0f;     // X offset for centering the maze
    float offsetY = 0.0f;     // Y offset for centering the maze
    int maxCols = 0;          // Number of columns in the maze
    int maxRows = 0;          // Number of rows in the maze
    
    struct ExitCell {
        int row;
        int col;
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        int ballsCollected = 0;
    };
        
    // Box2D world and physics components
    std::unique_ptr<World> physicsWorld;
    float timeStep = 1.0f / 60.0f;
    int32_t velocityIterations = 6;
    int32_t positionIterations = 2;
    
    // Game-specific variables
    int score = 0;
    // Scale factor for Box2D (which uses meters)
    float pixelsPerMeter = 10.0f;
    bool isDragging = false;
    int draggedBallIndex = -1;
    b2Vec2 lastMousePos = {0.0f, 0.0f};
   
    std::string title;
    std::string version;
    std::string resourcePath;
    const int INIT_WINDOW_W, INIT_WINDOW_H;

    State state;
    
    // Splash screen texture
    Texture splashTexture;
    
    // Background maze generation
    std::vector<std::string> generatedMazes;

    // Camera for coordinate transformations
    std::unique_ptr<OrthographicCamera> camera;

    // Components using PIMPL idiom
    std::unique_ptr<Renderer> mazeRenderer;

    PhysicsGameImpl(std::string_view title, std::string_view version, std::string_view resourcePath, int w, int h)
        : title{ title }
        , version{ version }
        , resourcePath{ resourcePath }
        , INIT_WINDOW_W{ w }, INIT_WINDOW_H{ h }
        , state{ State::SPLASH }
        , camera{ std::make_unique<OrthographicCamera>() }
        , mazeRenderer{ std::make_unique<Renderer>() } {

    }

    ~PhysicsGameImpl() {


    }

    std::optional<Resources> loadResources() {

        return ResourceManager::instance()->initializeAllResources(this->resourcePath);
    }
}; // impl

PhysicsGame::PhysicsGame(std::string_view title, std::string_view version, std::string_view resourcePath, int w, int h)
    : m_impl{ std::make_unique<PhysicsGameImpl>(title, version, resourcePath, w, h)} {
}

PhysicsGame::PhysicsGame(const std::string& title, const std::string& version, int w, int h)
    : PhysicsGame(std::string_view(title), std::string_view(version), ResourceManager::COMMON_RESOURCE_PATH_PREFIX, w, h) {}

PhysicsGame::~PhysicsGame() = default;

// Main game loop
bool PhysicsGame::run() const noexcept {
    
    using std::async;
    using std::launch;
    using std::make_unique;
    using std::move;
    using std::optional;
    using std::ref;
    using std::string;
    using std::string_view;
    using std::unique_ptr;
    using std::vector;

    auto&& gamePtr = this->m_impl;
    auto&& sdlHelper = mazes::singleton_base<SDLHelper>::instance();

    // Initialize SDL first
    sdlHelper->init();

    auto windowTitle = gamePtr->title + " - " + gamePtr->version;
    sdlHelper->createWindowAndRenderer(windowTitle, gamePtr->INIT_WINDOW_W, gamePtr->INIT_WINDOW_H);

    // Check if window and renderer were created successfully
    if (!sdlHelper->window || !sdlHelper->renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create SDL window or renderer");
        return false;
    }

    SDL_Log("Successfully created SDL window and renderer");

    // Now load resources after SDL is initialized
    auto resources = gamePtr->loadResources();
    if (!resources.has_value() || !resources.value().success) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load game resources");
        return false;
    }

    SDL_Log("Successfully loaded all game resources");

    // Load splash texture if we have a path
    if (!resources.value().splashPath.empty()) {
        if (gamePtr->splashTexture.loadFromFile(sdlHelper->renderer, resources.value().splashPath)) {
            SDL_Log("Successfully loaded splash texture: %s (%dx%d)", 
                resources.value().splashPath.c_str(), 
                gamePtr->splashTexture.getWidth(), 
                gamePtr->splashTexture.getHeight());
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load splash texture: %s", resources.value().splashPath.c_str());
        }
    }

    sf::SoundBuffer generateSoundBuffer;

    // Load and set window icon from resources
    auto iconPath = resources.value().windowIconPath;

    if (!iconPath.empty()) {
        SDL_Surface* icon = SDL_LoadBMP(iconPath.c_str());
        if (icon) {
            SDL_SetWindowIcon(sdlHelper->window, icon);
            SDL_DestroySurface(icon);
            SDL_Log("Successfully loaded window icon: %s", iconPath.c_str());
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load icon: %s - %s", iconPath.c_str(), SDL_GetError());
        }
    } else {
        SDL_Log("No window icon specified in configuration");
    }

    auto&& renderer = sdlHelper->renderer;
    SDL_SetRenderVSync(renderer, true);
    auto&& window = sdlHelper->window;

    vector<SDL_Vertex> level;
    
    // Create a static persistent string to store the maze data
    static std::string persistentMazeStr;
    
    // Set a good default value for pixelsPerMeter
    this->m_impl->pixelsPerMeter = 20.0f;
    
    // Start background maze generation while resources are loading
    // Maze myMaze{};
    // myMaze.startBackgroundMazeGeneration();

    // Generate initial level at startup
    int display_w, display_h;
    SDL_GetWindowSize(window, &display_w, &display_h);
    // this->m_impl->generateNewLevel(ref(persistentMazeStr), display_w, display_h);
    
    // Initial physics simulation step to ensure bodies are positioned
    if (this->m_impl->physicsWorld) {
        SDL_Log("Performing initial physics step");
        this->m_impl->physicsWorld->step(this->m_impl->timeStep, 4);
    }

    double previous = static_cast<double>(SDL_GetTicks());
    double accumulator = 0.0, currentTimeStep = 0.0;
    gamePtr->state = State::SPLASH;

    using GameObjects = std::variant<Ball, Wall>;

    vector<GameObjects> entities;
    entities.emplace_back(Ball{});
    entities.emplace_back(Wall{});
    entities.emplace_back(Wall{});


    SDL_Log("Starting main game loop in SPLASH state");
    
    while (gamePtr->state != State::DONE) {

        static constexpr auto FIXED_TIME_STEP = 1.0 / 60.0;
        auto elapsed = static_cast<double>(SDL_GetTicks()) - previous;
        previous = static_cast<double>(SDL_GetTicks());
        accumulator += elapsed;
        
        // Handle events and update physics at a fixed time step
        while (accumulator >= FIXED_TIME_STEP) {
            sdlHelper->poll_events(ref(gamePtr->state), ref(this->m_impl->camera));
            accumulator -= FIXED_TIME_STEP;
            currentTimeStep += FIXED_TIME_STEP;
        }

        // Update physics simulation if we're in a playing state
        if ((gamePtr->state == State::PLAY_SINGLE_MODE || gamePtr->state == State::PLAY_MULTI_MODE) && this->m_impl->physicsWorld) {
             
            // Step Box2D world with sub-steps instead of velocity/position iterations
            // Using 4 sub-steps as recommended in the migration guide
            // this->m_impl->physicsWorld->step(FIXED_TIME_STEP, 4);
        }

        // sdlHelper->updateAudioData();

        // Get window dimensions
        SDL_GetWindowSize(window, &display_w, &display_h);
        
        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // Light gray background
        SDL_RenderClear(renderer);
        
        // Render splash screen if in SPLASH state
        if (gamePtr->state == State::SPLASH && this->m_impl->splashTexture.get()) {
            // Center the splash image on screen
            int centerX = (display_w - this->m_impl->splashTexture.getWidth()) / 2;
            int centerY = (display_h - this->m_impl->splashTexture.getHeight()) / 2;
            
            SDL_FRect splashRect = {
                static_cast<float>(centerX),
                static_cast<float>(centerY),
                static_cast<float>(this->m_impl->splashTexture.getWidth()),
                static_cast<float>(this->m_impl->splashTexture.getHeight())
            };
            
            SDL_RenderTexture(renderer, this->m_impl->splashTexture.get(), nullptr, &splashRect);
            
            // Add a simple "Press any key to continue" text instruction
            // For now, we'll just continue after a short delay or key press
            // This can be enhanced later with proper text rendering
        }
        
        // Generate new level in MAIN_MENU state (only once when entering the state)
        static State previousState = State::SPLASH;
        if (gamePtr->state == State::MAIN_MENU && previousState != State::MAIN_MENU) {
            // Create and display maze with distances when first entering MAIN_MENU
        }
        previousState = gamePtr->state;
        
        // Start playing when transitioning from MAIN_MENU to PLAY_SINGLE_MODE
        if (gamePtr->state == State::PLAY_SINGLE_MODE && persistentMazeStr.empty()) {

        }
        
        // Draw the current maze if available (in PLAY_SINGLE_MODE or MAIN_MENU)
        if ((gamePtr->state == State::PLAY_SINGLE_MODE || gamePtr->state == State::MAIN_MENU)) {
            // Calculate centering offset

        }

        std::for_each(entities.begin(), entities.end(), [&elapsed](GameObjects& entity) {
            std::visit([&elapsed](auto&& arg) {
                arg.update(static_cast<float>(elapsed));
                arg.draw(static_cast<float>(elapsed));
            }, entity);
        });
        
        // Present the rendered frame
        SDL_RenderPresent(renderer);
        
        // FPS counter
        if (currentTimeStep >= 3000.0) {
            // Calculate frames per second
            SDL_Log("FPS: %d\n", static_cast<int>(1.0 / (elapsed / 1000.0)));
            // Calculate milliseconds per frame (correct formula)
            SDL_Log("Frame Time: %.3f ms/frame\n", elapsed);
            currentTimeStep = 0.0;
        }
    }
    
    return true;
}
