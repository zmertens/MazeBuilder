//
// PhysicsGame class implementation
// Simple 2D physics simulation with bouncy balls that break walls
// Navigate from start to finish in a time-sensitive race
//

#include "PhysicsGame.hpp"

#include <cmath>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string_view>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SFML/Audio.hpp>


#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/randomizer.h>

#include "GameState.hpp"
#include "LoadingState.hpp"
#include "MenuState.hpp"
#include "Player.hpp"
#include "PauseState.hpp"
#include "RenderWindow.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "SDLHelper.hpp"
#include "SplashState.hpp"
#include "State.hpp"
#include "StateStack.hpp"
#include "Texture.hpp"

#if defined(__EMSCRIPTEN__)

#include <emscripten_local/emscripten_mainloop_stub.h>
#endif

struct PhysicsGame::PhysicsGameImpl
{
    static constexpr auto COMMON_RESOURCE_PATH_PREFIX = "resources";

    // Game-specific constants
    static constexpr float WALL_HIT_THRESHOLD = 4.0f; // Number of hits before a wall breaks
    static constexpr float WALL_WIDTH = 0.1f;

    static constexpr int MAX_BALLS = 10;

    Player p1;

    std::unique_ptr<RenderWindow> window;

    SDLHelper sdlHelper;

    TextureManager textures;

    std::unique_ptr<StateStack> stateStack;

    // Game-specific variables
    int score = 0;

    std::string title;
    std::string version;
    std::string resourcePath;
    const int INIT_WINDOW_W, INIT_WINDOW_H;

    PhysicsGameImpl(std::string_view title, std::string_view version, std::string_view resourcePath, int w, int h)
        : title{title}
          , version{version}
          , resourcePath{resourcePath}
          , INIT_WINDOW_W{w}, INIT_WINDOW_H{h}
          , window{nullptr}
          , sdlHelper{}
          , stateStack{nullptr}
    {
        initSDL();

        // Initialize RenderWindow AFTER SDL renderer is created
        window = std::make_unique<RenderWindow>(sdlHelper.renderer, sdlHelper.window);

        // Initialize StateStack AFTER RenderWindow is created
        stateStack = std::make_unique<StateStack>(State::Context{*window, textures, p1});

        // Load initial textures needed for loading/splash screens
        loadSplashTextures();

        registerStates();

        // Push loading state with resource path, then splash state on top
        stateStack->pushState(States::ID::LOADING);
        stateStack->pushState(States::ID::SPLASH);
    }

    void initSDL() noexcept
    {
        auto windowTitle = title + " - " + version;
        sdlHelper.init(windowTitle, INIT_WINDOW_W, INIT_WINDOW_H);

        // Check if window and renderer were created successfully
        if (!sdlHelper.window || !sdlHelper.renderer)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create SDL window or renderer");

            return;
        }

        SDL_Log("Successfully created SDL window and renderer: %s (%dx%d)\n",
                windowTitle.c_str(), INIT_WINDOW_W, INIT_WINDOW_H);
    }

    void loadSplashTextures() noexcept
    {
        using std::string;

        // Configure and generate maze for loading/splash screens
        mazes::configurator config{};
        config.rows(INIT_WINDOW_H).columns(INIT_WINDOW_W).levels(1).algo_id(mazes::algo::BINARY_TREE).seed(42);

        try
        {
            // Generate maze and create texture from it
            SDL_Log("DEBUG: Generating initial maze with dimensions %dx%d", config.rows(), config.columns());
            string mazeString = mazes::create(config);

            if (!mazeString.empty())
            {
                SDL_Log("DEBUG: Maze generated successfully, length: %zu characters", mazeString.size());

                // Load the maze texture from the generated string
                constexpr int cellSize = 4; // Pixels per character in the maze
                textures.load(sdlHelper.renderer, Textures::ID::SPLASH_SCREEN, mazeString, cellSize);

                auto& mazeTexture = textures.get(Textures::ID::SPLASH_SCREEN);
                SDL_Log("DEBUG: Maze texture created successfully: %dx%d",
                        mazeTexture.getWidth(), mazeTexture.getHeight());
            }
            else
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to generate maze string");
            }
        }
        catch (const std::exception& e)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load initial textures: %s", e.what());
        }
    }

    void processInput()
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            // Let the state stack handle events
            stateStack->handleEvent(event);

            if (event.type == SDL_EVENT_QUIT)
            {
                stateStack->clearStates();
                break;
            }
        }
    }

    void update(float dt, int subSteps = 4)
    {
        stateStack->update(dt);
    }

    void render() const noexcept
    {
        // Clear, draw, and present (like SFML)
        window->clear();
        stateStack->draw();
        window->display();
    }

    void registerStates() noexcept
    {
        stateStack->registerState<GameState>(States::ID::GAME);
        stateStack->registerState<LoadingState>(States::ID::LOADING, resourcePath);
        stateStack->registerState<MenuState>(States::ID::MENU);
        stateStack->registerState<PauseState>(States::ID::PAUSE);
        stateStack->registerState<SplashState>(States::ID::SPLASH);
    }
}; // impl

PhysicsGame::PhysicsGame(std::string_view title, std::string_view version, std::string_view resourcePath, int w, int h)
    : m_impl{std::make_unique<PhysicsGameImpl>(title, version, resourcePath, w, h)}
{
}

PhysicsGame::PhysicsGame(const std::string& title, const std::string& version, int w, int h)
    : PhysicsGame(std::string_view(title), std::string_view(version), PhysicsGameImpl::COMMON_RESOURCE_PATH_PREFIX, w,
                  h)
{
}

PhysicsGame::~PhysicsGame() = default;

// Main game loop
bool PhysicsGame::run([[maybe_unused]] mazes::grid_interface* g, mazes::randomizer& rng) const noexcept
{
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

    SDL_Log("Entering game loop...\n");

    // Apply pending state changes (push SPLASH state onto stack)
    gamePtr->stateStack->update(0.0f);

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (gamePtr->window->isOpen())
#endif
    {
        // Expected milliseconds per frame (16.67ms)
        static constexpr auto FIXED_TIME_STEP = 1000.0 / 60.0;
        auto current = static_cast<double>(SDL_GetTicks());
        auto elapsed = current - previous;
        previous = current;
        accumulator += elapsed;

        // Handle events and update physics at a fixed time step
        while (accumulator >= FIXED_TIME_STEP)
        {
            gamePtr->processInput();

            accumulator -= FIXED_TIME_STEP;

            currentTimeStep += FIXED_TIME_STEP;

            gamePtr->update(static_cast<float>(FIXED_TIME_STEP) / 1000.f);
        }

        if (gamePtr->stateStack->isEmpty())
        {
            gamePtr->window->close();
        }

        gamePtr->render();

        // Cap frame rate at 60 FPS if VSync doesn't work
        if (auto frameTime = static_cast<double>(SDL_GetTicks()) - current; frameTime < FIXED_TIME_STEP)
        {
            SDL_Delay(static_cast<std::uint32_t>(FIXED_TIME_STEP - frameTime));
        }

        // FPS counter
        if (currentTimeStep >= 1000.0)
        {
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

void PhysicsGame::cleanup() noexcept
{
    if (auto& textures = this->m_impl->textures; !textures.isEmpty())
    {
        textures.clear();
        SDL_Log("PhysicsGame::cleanup() - Cleared texture manager\n");
    }

    if (auto& sdl = this->m_impl->sdlHelper; sdl.window || sdl.renderer)
    {
        sdl.destroyAndQuit();
        SDL_Log("PhysicsGame::cleanup() - Cleaning up SDL resources\n");
    }
}
