//
// PhysicsGame class implementation
// Simple 2D physics simulation with bouncy balls that break walls
//
// Audio Handling reference from SDL_AUDIO_STREAM: SDL\test\testaudio.c
//
// Score system: 10 points per wall destroyed
//              -1 point per friendly ball bounce
//              100 points per exit bounce
//

#include "PhysicsGame.hpp"

#include "CoutThreadSafe.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string_view>
#include <string>
#include <vector>

#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include <SFML/Audio.hpp>

#include <stb/stb_image.h>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_opengl3.h>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/json_helper.h>

#include "AudioHelper.hpp"
#include "Ball.hpp"
#include "Maze.hpp"
#include "OrthographicCamera.hpp"
#include "Physics.hpp"
#include "Renderer.hpp"
#include "ResourceManager.hpp"
#include "SDLHelper.hpp"
#include "State.hpp"
#include "Texture.hpp"
#include "Wall.hpp"
#include "WorkerConcurrent.hpp"
#include "World.hpp"

static const std::string RESOURCE_PATH_PREFIX = "resources";
static const std::string PHYSICS_JSON_PATH = RESOURCE_PATH_PREFIX + "/" + "physics.json";

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
    std::vector<Wall> walls;
    std::vector<Ball> balls;
    ExitCell exitCell;
    int score = 0;
    float pixelsPerMeter = 10.0f; // Scale factor for Box2D (which uses meters)
    bool isDragging = false;
    int draggedBallIndex = -1;
    b2Vec2 lastMousePos = {0.0f, 0.0f};
   
    const std::string& title;
    const std::string& version;
    const int INIT_WINDOW_W, INIT_WINDOW_H;

    State state;
    WorkerConcurrent workerConcurrent;
    
    // Splash screen texture
    SDL_Texture* splashTexture = nullptr;
    int splashWidth = 0, splashHeight = 0;
    
    // Maze distance data
    // cell index -> base36 distance character
    std::unordered_map<int, char> distanceMap;
    SDL_Texture* mazeDistanceTexture = nullptr;
    int mazeWidth = 0, mazeHeight = 0;
    
    // Background maze generation
    std::vector<std::string> generatedMazes;
    
    // Current active maze
    std::unique_ptr<Maze> currentMaze;

    // Camera for coordinate transformations
    std::unique_ptr<OrthographicCamera> camera;

    // Components using PIMPL idiom
    std::unique_ptr<Physics> physics;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<Renderer> mazeRenderer;

    PhysicsGameImpl(const std::string& title, const std::string& version, int w, int h)
        : title{ title }, version{ version }, INIT_WINDOW_W{ w }, INIT_WINDOW_H{ h }
        , state{ State::SPLASH }, workerConcurrent{state}
        , camera{ std::make_unique<OrthographicCamera>() }
        , physics{ std::make_unique<Physics>() }
        , resourceManager{ std::make_unique<ResourceManager>() }
        , mazeRenderer{ std::make_unique<Renderer>() } {

    }

    ~PhysicsGameImpl() {
        // Clean up splash texture
        if (splashTexture) {
            SDL_DestroyTexture(splashTexture);
            splashTexture = nullptr;
        }
        
        // Clean up maze distance texture
        if (mazeDistanceTexture) {
            SDL_DestroyTexture(mazeDistanceTexture);
            mazeDistanceTexture = nullptr;
        }
    }

    void generateNewLevel(std::string& persistentMazeStr, int display_w, int display_h) const {
        // Use Renderer component to generate maze
        persistentMazeStr = this->mazeRenderer->generateNewLevel(mazes::configurator::DEFAULT_ROWS, 
            mazes::configurator::DEFAULT_COLUMNS, display_w, display_h);
        
        // Calculate cell size
        float cellW = static_cast<float>(display_w) / static_cast<float>(mazes::configurator::DEFAULT_COLUMNS);
        float cellH = static_cast<float>(display_h) / static_cast<float>(mazes::configurator::DEFAULT_ROWS);
        float cellSize = std::min(cellW, cellH);
        
        // Use Physics component to create physics objects for the maze
        this->physics->createMazePhysics(persistentMazeStr, cellSize, display_w, display_h);
        
        SDL_Log("New level generated successfully using components");
    }
}; // impl

PhysicsGame::PhysicsGame(const std::string& title, const std::string& version, int w, int h)
    : m_impl{ std::make_unique<PhysicsGameImpl>(std::cref(title), std::cref(version), w, h)} {
}

PhysicsGame::~PhysicsGame() = default;

// Main game loop
bool PhysicsGame::run() const noexcept {
    using namespace std;

    auto&& gamePtr = this->m_impl;

    auto&& sdlHelper = mazes::singleton_base<SDLHelper>::instance();
    auto&& workers = gamePtr->workerConcurrent;

    sdlHelper->init();

    auto windowTitle = gamePtr->title + " - " + gamePtr->version;
    sdlHelper->createWindowAndRenderer(windowTitle, gamePtr->INIT_WINDOW_W, gamePtr->INIT_WINDOW_H);

    // Workers
    workers.initThreads();
    workers.generate("12345");

    // Load physics.json configuration using ResourceManager component
    if (!this->m_impl->resourceManager->loadConfiguration(PHYSICS_JSON_PATH)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load physics.json from: %s\n", PHYSICS_JSON_PATH.c_str());
        return false;
    }

    // Load audio resources using configuration
    const auto musicOggConfig = this->m_impl->resourceManager->getConfigValue("music_ogg");
    const auto generateOggFile = this->m_impl->resourceManager->extractJsonValue(musicOggConfig);
    sf::SoundBuffer generateSoundBuffer;
    const auto generatePath = this->m_impl->resourceManager->getResourcePath(generateOggFile);
    if (!generateSoundBuffer.loadFromFile(generatePath)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load sound file: %s\n", generatePath.c_str());
        return false;
    } else {
        SDL_Log("Success loading sound file: %s\n", generatePath.c_str());
    }

    unique_ptr<sf::Sound> generateSound = make_unique<sf::Sound>(generateSoundBuffer);
    AudioHelper audioHelper{cref(generateSound)};

    // Load and set window icon from configuration
    const auto iconImageConfig = this->m_impl->resourceManager->getConfigValue("icon_image");
    const auto iconImageFile = this->m_impl->resourceManager->extractJsonValue(iconImageConfig);
    const auto iconPath = this->m_impl->resourceManager->getResourcePath(iconImageFile);
    SDL_Surface* icon = SDL_LoadBMP(iconPath.c_str());
    if (icon) {
        SDL_SetWindowIcon(sdlHelper->window, icon);
        SDL_DestroySurface(icon);
        SDL_Log("Successfully loaded icon: %s\n", iconPath.c_str());
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load icon: %s - %s\n", iconPath.c_str(), SDL_GetError());
    }

    // Load WAV file from configuration
    const auto musicWavConfig = this->m_impl->resourceManager->getConfigValue("music_wav");
    const auto loadingWavFile = this->m_impl->resourceManager->extractJsonValue(musicWavConfig);
    const auto loadingPath = this->m_impl->resourceManager->getResourcePath(loadingWavFile);
    if (!sdlHelper->loadWAV(loadingPath.c_str())) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load WAV file: %s\n", loadingPath.c_str());
    } else {
        SDL_Log("Successfully loaded WAV file: %s\n", loadingPath.c_str());
    }

    sdlHelper->playAudioStream();

    // Load splash image from configuration
    const auto splashImageConfig = this->m_impl->resourceManager->getConfigValue("splash_image");
    const auto splashImageFile = this->m_impl->resourceManager->extractJsonValue(splashImageConfig);
    const auto splashImagePath = this->m_impl->resourceManager->getResourcePath(splashImageFile);
    this->m_impl->splashTexture = this->m_impl->resourceManager->loadTexture(sdlHelper->renderer, splashImagePath);
    if (this->m_impl->splashTexture) {
        // Get texture dimensions
        float width, height;
        SDL_GetTextureSize(this->m_impl->splashTexture, &width, &height);
        this->m_impl->splashWidth = static_cast<int>(width);
        this->m_impl->splashHeight = static_cast<int>(height);
        SDL_Log("Successfully loaded splash image: %s (%dx%d)\n", splashImagePath.c_str(), 
                this->m_impl->splashWidth, this->m_impl->splashHeight);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load splash image: %s\n", splashImagePath.c_str());
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
    Maze myMaze{};
    myMaze.startBackgroundMazeGeneration();

    // Generate initial level at startup
    int display_w, display_h;
    SDL_GetWindowSize(window, &display_w, &display_h);
    this->m_impl->generateNewLevel(ref(persistentMazeStr), display_w, display_h);
    
    // Log physics world state after level generation
    SDL_Log("PhysicsGame world created, num walls: %zu, num balls: %zu", 
        this->m_impl->walls.size(), this->m_impl->balls.size());
    
    // Initial physics simulation step to ensure bodies are positioned
    if (this->m_impl->physicsWorld) {
        SDL_Log("Performing initial physics step");
        this->m_impl->physicsWorld->step(this->m_impl->timeStep, 4);
    }

    double previous = static_cast<double>(SDL_GetTicks());
    double accumulator = 0.0, currentTimeStep = 0.0;
    gamePtr->state = State::SPLASH;
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
            this->m_impl->physicsWorld->step(FIXED_TIME_STEP, 4);
        }

        sdlHelper->updateAudioData();

        // Get window dimensions
        SDL_GetWindowSize(window, &display_w, &display_h);
        
        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // Light gray background
        SDL_RenderClear(renderer);
        
        // Render splash screen if in SPLASH state
        if (gamePtr->state == State::SPLASH && this->m_impl->splashTexture) {
            // Center the splash image on screen
            int centerX = (display_w - this->m_impl->splashWidth) / 2;
            int centerY = (display_h - this->m_impl->splashHeight) / 2;
            
            SDL_FRect splashRect = {
                static_cast<float>(centerX),
                static_cast<float>(centerY),
                static_cast<float>(this->m_impl->splashWidth),
                static_cast<float>(this->m_impl->splashHeight)
            };
            
            SDL_RenderTexture(renderer, this->m_impl->splashTexture, nullptr, &splashRect);
            
            // Add a simple "Press any key to continue" text instruction
            // For now, we'll just continue after a short delay or key press
            // This can be enhanced later with proper text rendering
        }
        
        // Render maze distance visualization in MAIN_MENU state
        if (gamePtr->state == State::MAIN_MENU && this->m_impl->mazeDistanceTexture) {
            // Center the maze distance texture on screen
            int centerX = (display_w - this->m_impl->mazeWidth) / 2;
            int centerY = (display_h - this->m_impl->mazeHeight) / 2;
            
            SDL_FRect mazeRect = {
                static_cast<float>(centerX),
                static_cast<float>(centerY),
                static_cast<float>(this->m_impl->mazeWidth),
                static_cast<float>(this->m_impl->mazeHeight)
            };
            
            SDL_RenderTexture(renderer, this->m_impl->mazeDistanceTexture, nullptr, &mazeRect);
            
            // Add instruction text (could be enhanced with proper text rendering)
            // For now, we'll just show the distance visualization
        }
        
        // Generate new level in MAIN_MENU state (only once when entering the state)
        static State previousState = State::SPLASH;
        if (gamePtr->state == State::MAIN_MENU && previousState != State::MAIN_MENU) {
            // Create and display maze with distances when first entering MAIN_MENU

            
            // Check if background maze generation is complete
            if (myMaze.isReady() && !this->m_impl->generatedMazes.empty()) {
                // Use the first generated maze
                int rows, cols;
                auto mazeCells = myMaze.parse(this->m_impl->generatedMazes[0], rows, cols);

                if (!mazeCells.empty()) {
                    // Initialize current maze with parsed data
                    this->m_impl->currentMaze = std::make_unique<Maze>();
                    float cellSize = std::min(display_w / (float)cols, display_h / (float)rows) * 0.8f; // 80% of available space
                    this->m_impl->currentMaze->initialize(renderer, mazeCells, rows, cols, cellSize);
                    
                    SDL_Log("Initialized maze rendering with %dx%d maze", rows, cols);
                }
            }
        }
        previousState = gamePtr->state;
        
        // Start playing when transitioning from MAIN_MENU to PLAY_SINGLE_MODE
        if (gamePtr->state == State::PLAY_SINGLE_MODE && persistentMazeStr.empty()) {
            // Generate initial level if we don't have one
            this->m_impl->generateNewLevel(ref(persistentMazeStr), display_w, display_h);
        }
        
        // Draw the current maze if available (in PLAY_SINGLE_MODE or MAIN_MENU)
        if ((gamePtr->state == State::PLAY_SINGLE_MODE || gamePtr->state == State::MAIN_MENU) && this->m_impl->currentMaze) {
            // Calculate centering offset
            float offsetX = (display_w - this->m_impl->currentMaze->getCols() * this->m_impl->currentMaze->getCellSize()) / 2.0f;
            float offsetY = (display_h - this->m_impl->currentMaze->getRows() * this->m_impl->currentMaze->getCellSize()) / 2.0f;
            
            this->m_impl->currentMaze->draw(renderer, this->m_impl->camera, 
                                           this->m_impl->pixelsPerMeter, offsetX, offsetY, 
                                           this->m_impl->currentMaze->getCellSize(), display_w, display_h);
        }
        
        // Present the rendered frame
        SDL_RenderPresent(renderer);
        
        // FPS counter
        if (currentTimeStep >= 1000.0) {
            // Calculate frames per second
            SDL_Log("FPS: %d\n", static_cast<int>(1.0 / (elapsed / 1000.0)));
            // Calculate milliseconds per frame (correct formula)
            SDL_Log("Frame Time: %.3f ms/frame\n", elapsed);
            // Log physics world status
            SDL_Log("Walls: %zu, Balls: %zu", this->m_impl->walls.size(), this->m_impl->balls.size());
            currentTimeStep = 0.0;
        }
    }
    
    return true;
}
