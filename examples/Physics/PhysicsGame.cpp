//
// PhysicsGame class implementation
// Simple 2D physics simulation with bouncy balls that break walls
//
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

#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/randomizer.h>

#include "AudioHelper.hpp"
#include "Ball.hpp"
#include "CoutThreadSafe.hpp"
#include "Drawable.hpp"
#include "JsonUtils.hpp"
#include "OrthographicCamera.hpp"
#include "Physical.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "SDLHelper.hpp"
#include "State.hpp"
#include "Texture.hpp"
#include "Wall.hpp"
#include "WorkerConcurrent.hpp"
#include "World.hpp"

#if defined(__EMSCRIPTEN__)
#include <emscripten_local/emscripten_mainloop_stub.h>
#endif

struct PhysicsGame::PhysicsGameImpl {

    static constexpr auto COMMON_RESOURCE_PATH_PREFIX = "resources";
    
    // Game-specific constants
    static constexpr float WALL_HIT_THRESHOLD = 4.0f; // Number of hits before a wall breaks
    static constexpr float WALL_WIDTH = 0.1f;

    static constexpr int MAX_BALLS = 10;
        
    World world;

    float timeStep;
    
    // Game-specific variables
    int score = 0;
   
    std::string title;
    std::string version;
    std::string resourcePath;
    const int INIT_WINDOW_W, INIT_WINDOW_H;

    State state;

    PhysicsGameImpl(std::string_view title, std::string_view version, std::string_view resourcePath, int w, int h)
        : timeStep{ 1.f }
        , title{ title}
        , version{ version }
        , resourcePath{ resourcePath }
        , INIT_WINDOW_W{ w }, INIT_WINDOW_H{ h }
        , state{ State::SPLASH } {

    }

    ~PhysicsGameImpl() {


    }

    void render() const {

        world.draw();
    }
}; // impl

PhysicsGame::PhysicsGame(std::string_view title, std::string_view version, std::string_view resourcePath, int w, int h)
    : m_impl{ std::make_unique<PhysicsGameImpl>(title, version, resourcePath, w, h)} {
}

PhysicsGame::PhysicsGame(const std::string& title, const std::string& version, int w, int h)
    : PhysicsGame(std::string_view(title), std::string_view(version), PhysicsGameImpl::COMMON_RESOURCE_PATH_PREFIX, w, h) {}

PhysicsGame::~PhysicsGame() = default;

// Main game loop
bool PhysicsGame::run([[maybe_unused]] mazes::grid_interface* g, mazes::randomizer& rng) const noexcept {
    
    using std::async;
    using std::launch;
    using std::make_unique;
    using std::move;
    using std::optional;
    using std::ref;
    using std::string;
    using std::string_view;
    using std::unique_ptr;
    using std::unordered_map;
    using std::vector;

    auto&& gamePtr = this->m_impl;
    auto&& gameWorld = this->m_impl->world;
    auto&& gameState = this->m_impl->state;
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

    auto&& renderer = sdlHelper->renderer;
    SDL_SetRenderVSync(renderer, true);
    auto&& window = sdlHelper->window;

    gameWorld.init();
    
    // Start background maze generation while resources are loading
    // Maze myMaze{};
    // myMaze.startBackgroundMazeGeneration();

    int display_w, display_h;
    SDL_GetWindowSize(window, &display_w, &display_h);
    
    // Initial physics simulation step to ensure bodies are positioned
    gameWorld.step(this->m_impl->timeStep, 4);

    double previous = static_cast<double>(SDL_GetTicks());
    double accumulator = 0.0, currentTimeStep = 0.0;
    gameState = State::SPLASH;

    SDL_Log("Starting main game loop in SPLASH state");

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (gameState != State::DONE)
#endif
    {
        static constexpr auto FIXED_TIME_STEP = 1.0 / 60.0;
        auto elapsed = static_cast<double>(SDL_GetTicks()) - previous;
        previous = static_cast<double>(SDL_GetTicks());
        accumulator += elapsed;
        
        // Handle events and update physics at a fixed time step
        while (accumulator >= FIXED_TIME_STEP) {

            sdlHelper->poll_events(ref(gameState));

            accumulator -= FIXED_TIME_STEP;

            currentTimeStep += FIXED_TIME_STEP;
        }

        // sdlHelper->updateAudioData();

        // Get window dimensions
        SDL_GetWindowSize(window, &display_w, &display_h);
        
        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // Light gray background
        SDL_RenderClear(renderer);
        
        // Draw the current maze if available (in PLAY_SINGLE_MODE or MAIN_MENU)
        if ((gameState == State::PLAY_SINGLE_MODE || gameState == State::MAIN_MENU)) {
            // Calculate centering offset

        }

        gamePtr->render();
        
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

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_END;
    emscripten_cancel_main_loop();
#endif
    
    return true;
}
