//
// physics_game class implementation
// Simple 2D physics simulation with bouncy balls that break walls
// Navigate from start to finish in a time-sensitive race
//

#include "physics_game.h"

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


#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create.h>

#include "render_window.h"
#include "sdl_helper.h"
#include "texture.h"

#if defined(__EMSCRIPTEN__)

#include <emscripten_local/emscripten_mainloop_stub.h>
#endif

struct physics_game::physics_game_impl
{
    static constexpr auto COMMON_RESOURCE_PATH_PREFIX = "resources";

    // Game-specific constants
    static constexpr float WALL_HIT_THRESHOLD = 4.0f; // Number of hits before a wall breaks
    static constexpr float WALL_WIDTH = 0.1f;

    static constexpr int MAX_BALLS = 10;

    std::unique_ptr<render_window> window;

    sdl_helper sdlHelper;

    // FPS smoothing variables
    mutable double fpsUpdateTimer = 0.0;
    mutable int smoothedFps = 0;
    mutable float smoothedFrameTime = 0.0f;
    static constexpr double FPS_UPDATE_INTERVAL = 250.0; // Update display every 250ms

    const std::string title, version;
    const int INIT_WINDOW_W, INIT_WINDOW_H;

    physics_game_impl(std::string_view title, std::string_view version, int w, int h)
        : title{title}
          , version{version}
          , INIT_WINDOW_W{w}, INIT_WINDOW_H{h}
          , window{nullptr}
          , sdlHelper{}
    {
        initSDL();

        // Check if SDL initialization succeeded
        if (!sdlHelper.window || !sdlHelper.renderer)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create SDL window or renderer - cannot continue");
            // Don't initialize further objects if SDL failed
            return;
        }

        this->window = std::make_unique<render_window>(sdlHelper.renderer, sdlHelper.window);

        initDearImGui();
    }

    ~physics_game_impl()
    {
        if (auto& sdl = this->sdlHelper; sdl.window || sdl.renderer)
        {
            sdl.destroyAndQuit();
        }
    }

    void initSDL() noexcept
    {
        auto windowTitle = title + " - " + version;
        this->sdlHelper.init(windowTitle, INIT_WINDOW_W, INIT_WINDOW_H);
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

    void processInput() const noexcept
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            // Let ImGui process the event first
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                this->window->close();
                break;
            }

            // Then let the state stack handle events
        }
    }

    void update(const float dt, int subSteps = 4) const noexcept
    {

    }

    void render(double& currentTimeStep, const double elapsed) const noexcept
    {
        // Clear, draw, and present (like SFML)
        window->clear();
        window->beginFrame();

#if defined(MAZE_DEBUG)
        // Window might be closed during draw calls/events
        if (window->isOpen())
        {
            this->handleFPS(std::ref(currentTimeStep), elapsed);
        }
#endif

        window->display();
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

physics_game::physics_game(std::string_view title, std::string_view version, int w, int h)
    : m_impl{std::make_unique<physics_game_impl>(title, version, w, h)}
{
}

physics_game::physics_game(const std::string& title, const std::string& version, int w, int h)
    : physics_game(std::string_view(title), std::string_view(version), w,
                  h)
{
}

physics_game::~physics_game() = default;

// Main game loop
bool physics_game::run([[maybe_unused]] mazes::grid_interface* g, mazes::randomizer& rng) const noexcept
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
    if (!gamePtr->window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Game initialization failed - cannot run");
        return false;
    }

    auto previous = static_cast<double>(SDL_GetTicks());
    double accumulator = 0.0, currentTimeStep = 0.0;

    SDL_Log("Entering game loop...\n");

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

        gamePtr->render(ref(currentTimeStep), elapsed);
    }

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_END;
    emscripten_cancel_main_loop();
#endif

    return true;
}

