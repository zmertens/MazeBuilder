//
// PhysicsGame class implementation
// Simple 2D physics simulation with bouncy balls that break walls
// Navigate from start to finish in a time-sensitive race
//

#include "PhysicsGame.hpp"

#include <cmath>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_sdlrenderer3.h>

#include "resources/Cousine_Regular.h"
#include "resources/Limelight_Regular.h"
#include "resources/nunito_sans.h"

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create.h>
#include <MazeBuilder/json_helper.h>

#include "Font.hpp"
#include "GameState.hpp"
#include "LoadingState.hpp"
#include "MenuState.hpp"
#include "Player.hpp"
#include "PauseState.hpp"
#include "SettingsState.hpp"
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

    FontManager fonts;

    TextureManager textures;

    std::unique_ptr<StateStack> stateStack;

    // Game-specific variables
    int score = 0;

    // FPS smoothing variables
    mutable double fpsUpdateTimer = 0.0;
    mutable int smoothedFps = 0;
    mutable float smoothedFrameTime = 0.0f;
    static constexpr double FPS_UPDATE_INTERVAL = 250.0; // Update display every 250ms

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

        // Check if SDL initialization succeeded
        if (!sdlHelper.window || !sdlHelper.renderer)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create SDL window or renderer - cannot continue");
            // Don't initialize further objects if SDL failed
            return;
        }

        window = std::make_unique<RenderWindow>(sdlHelper.renderer, sdlHelper.window);

        initDearImGui();

        stateStack = std::make_unique<StateStack>(State::Context{
            *window,
            std::ref(fonts),
            std::ref(textures),
            std::ref(p1)
        });

        // Load initial textures needed for loading/splash screens
        generateLevelOne();

        loadFonts();

        registerStates();

        // Push loading state with resource path, then splash state on top
        stateStack->pushState(States::ID::LOADING);
        stateStack->pushState(States::ID::SPLASH);
    }

    ~PhysicsGameImpl()
    {
        if (auto& sdl = this->sdlHelper; sdl.window || sdl.renderer)
        {
            this->stateStack->clearStates();
            this->fonts.clear();
            this->textures.clear();
            sdl.destroyAndQuit();
        }
    }

    void initSDL() noexcept
    {
        auto windowTitle = title + " - " + version;
        sdlHelper.init(windowTitle, INIT_WINDOW_W, INIT_WINDOW_H);
    }

    void initDearImGui() const noexcept
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
        ImGui::GetIO().IniFilename = nullptr;

        // Setup ImGui Platform/Renderer backends
        ImGui_ImplSDL3_InitForSDLRenderer(this->sdlHelper.window, this->sdlHelper.renderer);
        ImGui_ImplSDLRenderer3_Init(this->sdlHelper.renderer);
    }

    void generateLevelOne() noexcept
    {
        using std::string;

        mazes::configurator config{};
        config.rows(mazes::configurator::MAX_ROWS)
              .columns(mazes::configurator::MAX_COLUMNS)
              .levels(1)
              .algo_id(mazes::algo::BINARY_TREE)
              .seed(42)
              .distances(true)
              .distances_start(0)
              .distances_end(-1);

        try
        {
            if (const auto mazeStr = mazes::create(config); !mazeStr.empty())
            {
                // Load the maze texture from the generated string
                textures.load(sdlHelper.renderer, Textures::ID::LEVEL_ONE, mazeStr, 12);
            }
            else
            {
                throw std::runtime_error("Failed to create maze:\n" + mazeStr);
            }
        }
        catch (const std::exception& e)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to generate level one: %s", e.what());
        }
    }

    void loadFonts() noexcept
    {
        static constexpr auto FONT_PIXEL_SIZE = 28.f;
        fonts.load(Fonts::ID::LIMELIGHT,
            Limelight_Regular_compressed_data,
            Limelight_Regular_compressed_size,
                   FONT_PIXEL_SIZE);
        fonts.load(Fonts::ID::NUNITO_SANS,
            NunitoSans_compressed_data,
            NunitoSans_compressed_size,
            FONT_PIXEL_SIZE);
        fonts.load(Fonts::ID::COUSINE_REGULAR,
            Cousine_Regular_compressed_data,
            Cousine_Regular_compressed_size,
                   FONT_PIXEL_SIZE);

        // Build font atlas after adding fonts
        ImGui::GetIO().Fonts->Build();
    }

    void processInput() const noexcept
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            // Let ImGui process the event first
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                stateStack->clearStates();
                break;
            }

            // Then let the state stack handle events
            stateStack->handleEvent(event);
        }
    }

    void update(const float dt, int subSteps = 4) const noexcept
    {
        stateStack->update(dt, subSteps);
    }

    void render(double& currentTimeStep, const double elapsed) const noexcept
    {
        // Clear, draw, and present (like SFML)
        window->clear();
        window->beginFrame();
        stateStack->draw();

#if defined(MAZE_DEBUG)
        // Window might be closed during draw calls/events
        if (window->isOpen())
        {
            this->handleFPS(std::ref(currentTimeStep), elapsed);
        }
#endif

        window->display();
    }

    void registerStates() noexcept
    {
        stateStack->registerState<GameState>(States::ID::GAME);
        stateStack->registerState<LoadingState>(States::ID::LOADING, resourcePath);
        stateStack->registerState<MenuState>(States::ID::MENU);
        stateStack->registerState<PauseState>(States::ID::PAUSE);
        stateStack->registerState<SettingsState>(States::ID::SETTINGS);
        stateStack->registerState<SplashState>(States::ID::SPLASH);
    }

    void handleFPS(double& currentTimeStep, const double elapsed) const noexcept
    {
        // Calculate instantaneous FPS and frame time
        const auto fps = static_cast<int>(1000.0 / elapsed);
        const auto frameTime = static_cast<float>(elapsed);

        // Update smoothed values periodically for display
        fpsUpdateTimer += elapsed;
        if (fpsUpdateTimer >= FPS_UPDATE_INTERVAL)
        {
            smoothedFps = fps;
            smoothedFrameTime = frameTime;
            fpsUpdateTimer = 0.0;
        }

        // if (currentTimeStep >= 1000.0)
        // {
        //     SDL_Log("FPS: %d\n", smoothedFps);
        //     SDL_Log("Frame Time: %.3f ms/frame\n", smoothedFrameTime);
        //
        //     currentTimeStep = 0.0;
        // }

        // Create ImGui overlay window
        // Set window position to top-right corner
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

        // Set window background to be semi-transparent
        ImGui::SetNextWindowBgAlpha(0.65f);

        // Create window with no title bar, no resize, no move, auto-resize
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoFocusOnAppearing |
                                       ImGuiWindowFlags_NoNav |
                                       ImGuiWindowFlags_NoMove;

        if (ImGui::Begin("FPS Overlay", nullptr, windowFlags))
        {
            ImGui::Text("FPS: %d", smoothedFps);
            ImGui::Text("Frame Time: %.2f ms", smoothedFrameTime);
            ImGui::End();
        }
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
    using std::cref;
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

    // Check if initialization succeeded before entering game loop
    if (!gamePtr->window || !gamePtr->stateStack)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Game initialization failed - cannot run");
        return false;
    }

    auto previous = static_cast<double>(SDL_GetTicks());
    double accumulator = 0.0, currentTimeStep = 0.0;

    SDL_Log("Entering game loop...\n");

    // Apply pending state changes (push SPLASH state onto stack)
    gamePtr->stateStack->update(0.1f, 4);

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (gamePtr->window && gamePtr->window->isOpen())
#endif
    {
        // Expected milliseconds per frame (16.67ms)
        static constexpr auto FIXED_TIME_STEP = 1000.0 / 60.0;
        const auto current = static_cast<double>(SDL_GetTicks());
        const auto elapsed = current - previous;
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

        gamePtr->render(ref(currentTimeStep), elapsed);
    }

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_END;
    emscripten_cancel_main_loop();
#endif

    return true;
}

