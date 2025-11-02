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
#include "GameState.hpp"
#include "JsonUtils.hpp"
#include "Physical.hpp"
#include "Player.hpp"
#include "RenderWindow.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "SDLHelper.hpp"
#include "SplashState.hpp"
#include "State.hpp"
#include "StateStack.hpp"
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
        
    Player p1;

    std::unique_ptr<RenderWindow> window;

    TextureManager textures;

    std::unique_ptr<StateStack> stateStack;
    
    // Game-specific variables
    int score = 0;
   
    std::string title;
    std::string version;
    std::string resourcePath;
    const int INIT_WINDOW_W, INIT_WINDOW_H;

    PhysicsGameImpl(std::string_view title, std::string_view version, std::string_view resourcePath, int w, int h)
        : title{ title}
        , version{ version }
        , resourcePath{ resourcePath }
        , INIT_WINDOW_W{ w }, INIT_WINDOW_H{ h }
        , p1{}
        , window{nullptr}
        , textures{}
        , stateStack{nullptr} {

        initSDL();
        
        // Initialize RenderWindow AFTER SDL renderer is created
        window = std::make_unique<RenderWindow>(mazes::singleton_base<SDLHelper>::instance()->renderer);
        
        // Initialize StateStack AFTER RenderWindow is created
        stateStack = std::make_unique<StateStack>(State::Context{*window, textures, p1});
        
        loadTextures();
        registerStates();
        
        stateStack->pushState(States::ID::SPLASH);
    }

    void initSDL() noexcept {
        auto&& sdlHelper = mazes::singleton_base<SDLHelper>::instance();

        sdlHelper->init();

        auto windowTitle = title + " - " + version;
        sdlHelper->createWindowAndRenderer(windowTitle, INIT_WINDOW_W, INIT_WINDOW_H);

        // Check if window and renderer were created successfully
        if (!sdlHelper->window || !sdlHelper->renderer) {

            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create SDL window or renderer");

            return;
        }

        SDL_Log("Successfully created SDL window and renderer");
    }

    void loadTextures() noexcept {
        using std::string;
        using std::unordered_map;
        using std::ref;

        JsonUtils jsonUtils{};
        unordered_map<string, string> resources{};
        try {
            // Load resource configuration
            jsonUtils.loadConfiguration("resources/physics.json", ref(resources));
            SDL_Log(jsonUtils.getValue("splash_image", resources).c_str());
            auto splashImagePath = "resources/" + jsonUtils.getValue("splash_image", resources);
            SDL_Log("DEBUG: Loading splash screen from: %s", splashImagePath.c_str());
            textures.load(Textures::ID::SPLASH_SCREEN, splashImagePath);
            
            auto avatarValue = jsonUtils.getValue("avatar", resources);
            SDL_Log("DEBUG: Avatar value from JSON: '%s'", avatarValue.c_str());
            string avatarImagePath = "resources/character_beige_front.png";
            SDL_Log("DEBUG: Loading avatar from: %s", avatarImagePath.c_str());
            textures.load(Textures::ID::AVATAR, avatarImagePath);
        } catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load textures: %s", e.what());
            return;
        }

        SDL_Log("Successfully loaded all game resources");

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

    ~PhysicsGameImpl() {


    }

    void processInput() {

        SDL_Event event;

        while (SDL_PollEvent(&event)) {

            // Let the state stack handle events
            stateStack->handleEvent(event);

            if (event.type == SDL_EVENT_QUIT) {
                stateStack->clearStates();
                break;
            }
        }
    }

    void update(float dt, int subSteps = 4) {

        stateStack->update(dt);
    }

    void render() const noexcept {

        // Clear, draw, and present (like SFML)
        window->clear();
        stateStack->draw();
        window->display();
    }

    void registerStates() noexcept {

        stateStack->registerState<SplashState>(States::ID::SPLASH);
        stateStack->registerState<GameState>(States::ID::GAME);
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
    
    // Start background maze generation while resources are loading
    // Maze myMaze{};
    // myMaze.startBackgroundMazeGeneration();

    double previous = static_cast<double>(SDL_GetTicks());
    double accumulator = 0.0, currentTimeStep = 0.0;

    SDL_Log("Starting main game loop in SPLASH state");

    // Apply pending state changes (push SPLASH state onto stack)
    gamePtr->stateStack->update(0.0f);

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!gamePtr->stateStack->isEmpty())
#endif
    {
         // Expected milliseconds per frame (16.67ms)
        static constexpr auto FIXED_TIME_STEP = 1000.0 / 60.0;
        auto current = static_cast<double>(SDL_GetTicks());
        auto elapsed = current - previous;
        previous = current;
        accumulator += elapsed;
        
        // Handle events and update physics at a fixed time step
        while (accumulator >= FIXED_TIME_STEP) {

            gamePtr->processInput();

            accumulator -= FIXED_TIME_STEP;

            currentTimeStep += FIXED_TIME_STEP;

            gamePtr->update(static_cast<float>(FIXED_TIME_STEP) / 1000.f);
        }

        gamePtr->render();
        
        // Cap frame rate at 60 FPS if VSync doesn't work
        if (auto frameTime = static_cast<double>(SDL_GetTicks()) - current; frameTime < FIXED_TIME_STEP) {

            SDL_Delay(static_cast<std::uint32_t>(FIXED_TIME_STEP - frameTime));
        }
        
        // FPS counter
        if (currentTimeStep >= 1000.0) {

            // Calculate frames per second (elapsed is in milliseconds)
            SDL_Log("FPS: %d\n", static_cast<int>(1000.0 / elapsed));
            // Calculate milliseconds per frame
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
