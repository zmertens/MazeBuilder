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
#include <ranges>
#include <string_view>
#include <string>
#include <vector>

#include <box2d/box2d.h>

#include <SDL3/SDL.h>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_sdlrenderer3.h>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create.h>

#include "ball.h"
#include "mouse_states.h"
#include "render_window.h"
#include "sdl_helper.h"
#include "texture.h"
#include "wall.h"

#include "resources/Cousine_Regular.h"

#if defined(__EMSCRIPTEN__)

#include <emscripten_local/emscripten_mainloop_stub.h>
#endif

struct physics_game::physics_game_impl
{
    static constexpr std::string_view SDL_LOGO_FILEPATH{"resources/SDL_logo.png"};
    static constexpr std::string_view SFML_LOGO_FILEPATH{"resources/SFML_logo.png"};
    static constexpr std::string_view WINDOW_ICON_FILEPATH{"resources/icon.bmp"};

    // Game-specific constants
    static constexpr auto WALL_HIT_THRESHOLD = 4.0f;
    static constexpr auto WALL_WIDTH = 0.1f;

    static constexpr auto MAX_BALLS = 10;

    sdl_helper sdl_helper;

    std::unique_ptr<texture> sdl_logo;
    std::unique_ptr<texture> sfml_logo;

    std::unique_ptr<render_window> window;

    // FPS smoothing variables
    mutable double fps_update_timer = 0.0;
    mutable int smoothed_fps = 0;
    mutable float smoothed_frame_time = 0.0f;
    // Update display every 250ms
    static constexpr double FPS_UPDATE_INTERVAL = 250.0;

    const std::string title, version;
    const int INIT_WINDOW_W, INIT_WINDOW_H;

    // Maze rendering variables for physics coordinate system
    float cell_size = 0.0f;
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    int max_cols = 0;
    int max_rows = 0;

    // Box2D world and physics components
    b2BodyId boundaryBodyId = b2_nullBodyId;
    b2WorldId world_id = b2_nullWorldId;
    // Scale factor for Box2D (which uses meters)
    float pixels_per_meter = 40.0f;

    // Body tracking for rendering
    std::vector<ball> ball_bodies;
    std::vector<wall> wall_bodies;

    physics_game_impl(std::string_view title, std::string_view version, int w, int h)
        : title{title}
          , version{version}
          , INIT_WINDOW_W{w}, INIT_WINDOW_H{h}
          , window{nullptr}
          , sdl_helper{}
          , sdl_logo{std::make_unique<texture>()}
          , sfml_logo{std::make_unique<texture>()}
    {
        init_sdl();

        // Check if SDL initialization succeeded
        if (!sdl_helper.window || !sdl_helper.renderer)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create SDL window or renderer - cannot continue");
            // Don't initialize further objects if SDL failed
            return;
        }

        this->window = std::make_unique<render_window>(sdl_helper.renderer, sdl_helper.window);

        init_dear_imgui();

        init_textures();
    }

    ~physics_game_impl()
    {
        if (auto& sdl = this->sdl_helper; sdl.window || sdl.renderer)
        {
            sdl.destroyAndQuit();
        }
    }

    void init_sdl() noexcept
    {
        const auto windowTitle = title + " - " + version;
        this->sdl_helper.init(windowTitle, INIT_WINDOW_W, INIT_WINDOW_H);
    }

    void init_dear_imgui() const noexcept
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
        ImGui::GetIO().IniFilename = nullptr;

        // Setup ImGui Platform/Renderer backends
        ImGui_ImplSDL3_InitForSDLRenderer(this->sdl_helper.window, this->sdl_helper.renderer);
        ImGui_ImplSDLRenderer3_Init(this->sdl_helper.renderer);

        ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(Cousine_Regular_compressed_data,
                                                             Cousine_Regular_compressed_size,
                                                             pixels_per_meter);
    }

    void init_textures() const noexcept
    {
        if (!sdl_logo->loadFromFile(sdl_helper.renderer, SDL_LOGO_FILEPATH))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load SDL logo texture from %s",
                         SDL_LOGO_FILEPATH.data());
        }

        if (!sfml_logo->loadFromFile(sdl_helper.renderer, SFML_LOGO_FILEPATH))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load SFML logo texture from %s",
                         SFML_LOGO_FILEPATH.data());
        }

        if (SDL_Surface* bmp_surface = SDL_LoadBMP(WINDOW_ICON_FILEPATH.data()))
        {
            SDL_SetWindowIcon(sdl_helper.window, bmp_surface);
            SDL_DestroySurface(bmp_surface);
        }
        else
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load window icon from %s: %s",
                         WINDOW_ICON_FILEPATH.data(), SDL_GetError());
        }
    }

    void processInput() const noexcept
    {
        SDL_Event event;

        // Let ImGui process the event first
        ImGui_ImplSDL3_ProcessEvent(&event);

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                this->window->close();
                break;
            }
        }
    }

    void update(const float dt, mazes::randomizer& rng, int subSteps = 4) noexcept
    {
        // Step the Box2D physics world if initialized
        if (B2_IS_NON_NULL(world_id))
        {
            // Step physics with substeps for stability
            // Substeps help with collision accuracy at high velocities
            b2World_Step(world_id, dt, subSteps);

            // Process collision events and game logic
            process_physics_collisions();

            // Handle mouse dragging
            float mouse_x, mouse_y;
            const auto mouseState = SDL_GetMouseState(&mouse_x, &mouse_y);
            const auto [x, y] = screen_to_physics_coords(mouse_x, mouse_y);
            const bool is_mouse_down = (mouseState & SDL_BUTTON_LMASK) != 0;
            const mouse_states mice{
                is_mouse_down ? mouse_states::button_state::DOWN : mouse_states::button_state::UP,
                mouse_states::button_state::UP,
                static_cast<int>(x),
                static_cast<int>(y)
            };

            for (auto& b : ball_bodies)
            {
                b.update(dt, std::cref(mice), std::ref(rng));
            }
        }
    }

    void render(const double elapsed) noexcept
    {
        // Clear, draw, and present (like SFML)
        window->clear();
        window->begin_frame();

        // Render the physics world if initialized
        if (B2_IS_NON_NULL(world_id))
        {
            render_physics_world();
        }

#if defined(MAZE_DEBUG)
        // Window might be closed during draw calls/events
        if (window->is_open())
        {
            this->handle_fps(elapsed);
        }
#endif

        window->display();
    }

    void handle_fps(const double elapsed) const noexcept
    {
        // Calculate instantaneous FPS and frame time
        const auto fps = static_cast<int>(1000.0 / elapsed);
        const auto frameTime = static_cast<float>(elapsed);

        // Update smoothed values periodically for display
        fps_update_timer += elapsed;
        if (fps_update_timer >= FPS_UPDATE_INTERVAL)
        {
            smoothed_fps = fps;
            smoothed_frame_time = frameTime;
            fps_update_timer = 0.0;
        }

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

        // Create ImGui overlay window
        // Set window position to top-right corner
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Always,
                                ImVec2(1.0f, 0.0f));

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
            ImGui::Text("FPS: %d", smoothed_fps);
            ImGui::Text("Frame Time: %.2f ms", smoothed_frame_time);
            ImGui::End();
        }

        ImGui::PopFont();
    }

    /// @brief Initialize the Box2D physics world with gravity
    /// @details Creates a new Box2D world with downward gravity (9.8 m/s²) and
    ///          sets up simulation parameters for 60Hz physics updates.
    ///          This provides realistic physics for balls bouncing off walls.
    void init_physics() noexcept
    {
        // Destroy existing world if present
        if (B2_IS_NON_NULL(world_id))
        {
            b2DestroyWorld(world_id);
        }

        // Create Box2D world with gravity pointing down
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {0.0f, 9.8f};
        world_id = b2CreateWorld(&worldDef);

        // Set physics simulation parameters
        pixels_per_meter = 40.0f;

        // Clear body tracking
        wall_bodies.clear();
        ball_bodies.clear();
        boundaryBodyId = b2_nullBodyId;

        SDL_Log("Physics world initialized with gravity: (0.0, 9.8)");
    }

    /// @brief Convert screen coordinates to Box2D physics world coordinates
    /// @param screen_x X coordinate in screen/pixel space
    /// @param screen_y Y coordinate in screen/pixel space
    /// @return b2Vec2 position in physics world space (meters)
    /// @details This accounts for the maze offset and pixel-to-meter conversion.
    ///          Essential for mouse interaction with physics objects.
    b2Vec2 screen_to_physics_coords(float screen_x, float screen_y) const noexcept
    {
        // Convert from screen coordinates to physics coordinates
        // by accounting for offset and scale
        const float phys_x = (screen_x - offset_x) / pixels_per_meter;
        const float phys_y = (screen_y - offset_y) / pixels_per_meter;

        return {phys_x, phys_y};
    }

    /// @brief Convert Box2D physics coordinates to screen coordinates
    /// @param phys_x X position in physics world (meters)
    /// @param phys_y Y position in physics world (meters)
    /// @return SDL_FPoint position in screen/pixel space
    /// @details Used for rendering physics objects at correct screen positions
    SDL_FPoint physics_to_screen(float phys_x, float phys_y) const noexcept
    {
        return {phys_x * pixels_per_meter + offset_x, phys_y * pixels_per_meter + offset_y};
    }

    /// @brief Create maze physics objects from ASCII maze string
    /// @param maze_string ASCII representation of the maze ('+', '-', '|', ' ')
    /// @param cell_size_param Size of each cell in pixels
    /// @details Parses the maze string and creates Box2D static bodies for walls.
    ///          Walls are represented as:
    ///          - '+' : Corner joints (small squares)
    ///          - '-' : Horizontal walls
    ///          - '|' : Vertical walls
    ///          - ' ' : Empty path cells
    ///          Also creates boundary walls around the entire maze to contain balls.
    void create_physics_objects(std::string_view maze_string, float cell_size_param) noexcept
    {
        if (maze_string.empty())
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Cannot create maze physics from empty string");
            return;
        }

        // Initialize or reset physics world
        init_physics();

        // Calculate maze dimensions by parsing the string
        const char* mazeData = maze_string.data();
        size_t mazeLen = maze_string.size();
        int currentRow = 0;
        int colCount = 0;

        max_cols = 0;
        max_rows = 0;

        for (size_t i = 0; i < mazeLen; i++)
        {
            if (mazeData[i] == '\n')
            {
                max_cols = std::max(max_cols, colCount);
                colCount = 0;
                currentRow++;
            }
            else
            {
                colCount++;
            }
        }
        max_rows = currentRow + 1;

        SDL_Log("Maze dimensions: %d rows x %d columns", max_rows, max_cols);

        // Store cell size and calculate centering offsets
        cell_size = cell_size_param;
        pixels_per_meter = cell_size; // Use cell size as the meter scale

        float mazeWidth = max_cols * cell_size;
        float mazeHeight = max_rows * cell_size;
        offset_x = std::max(0.0f, (INIT_WINDOW_W - mazeWidth) / 2.0f);
        offset_y = std::max(0.0f, (INIT_WINDOW_H - mazeHeight) / 2.0f);

        // Calculate world dimensions in physics units (meters)
        float worldWidth = (max_cols * cell_size) / pixels_per_meter;
        float worldHeight = (max_rows * cell_size) / pixels_per_meter;

        SDL_Log("Physics world size: %.2f x %.2f meters", worldWidth, worldHeight);
        SDL_Log("Cell size: %.2f pixels, pixelsPerMeter: %.2f", cell_size, pixels_per_meter);

        // Create boundary walls around the entire world
        create_world_boundaries(worldWidth, worldHeight);

        // Parse maze and create wall bodies
        create_walls(maze_string);

        SDL_Log("Maze physics creation complete");
    }

    /// @brief Create static boundary walls around the physics world
    /// @param world_width Width of the world in meters
    /// @param world_height Height of the world in meters
    /// @details Creates 4 edge segments (top, bottom, left, right) to prevent
    ///          balls from escaping the play area. These are sensor boundaries.
    void create_world_boundaries(float world_width, float world_height) noexcept
    {
        // Create a static body for boundaries
        b2BodyDef boundaryDef = b2DefaultBodyDef();
        boundaryDef.type = b2_staticBody;
        boundaryDef.userData = reinterpret_cast<void*>(3000);
        boundaryBodyId = b2CreateBody(world_id, &boundaryDef);

        // Shape properties for boundaries
        b2ShapeDef boundaryShapeDef = b2DefaultShapeDef();
        boundaryShapeDef.density = 0.0f;
        // Note: In Box2D 3.0, friction and restitution are set differently
        // They can be set on the shape after creation if needed

        // Define boundary positions with a small margin
        constexpr float margin = 1.0f;
        constexpr float left = -margin;
        const float right = world_width + margin;
        constexpr float top = -margin;
        const float bottom = world_height + margin;

        // Top boundary
        b2Segment topSeg = {{left, top}, {right, top}};
        b2CreateSegmentShape(boundaryBodyId, &boundaryShapeDef, &topSeg);

        // Bottom boundary
        b2Segment bottomSeg = {{left, bottom}, {right, bottom}};
        b2CreateSegmentShape(boundaryBodyId, &boundaryShapeDef, &bottomSeg);

        // Left boundary
        b2Segment leftSeg = {{left, top}, {left, bottom}};
        b2CreateSegmentShape(boundaryBodyId, &boundaryShapeDef, &leftSeg);

        // Right boundary
        b2Segment rightSeg = {{right, top}, {right, bottom}};
        b2CreateSegmentShape(boundaryBodyId, &boundaryShapeDef, &rightSeg);

        SDL_Log("Created world boundaries at: (%.1f, %.1f) to (%.1f, %.1f)",
                left, top, right, bottom);
    }

    /// @brief Parse maze string and create Box2D wall bodies
    /// @param maze_string ASCII maze representation
    /// @details Creates static box-shaped bodies for each wall character.
    ///          - Horizontal walls ('-'): Full width, thin height
    ///          - Vertical walls ('|'): Thin width, full height
    ///          - Corners ('+'): Small square boxes
    ///          Walls are given unique user data IDs (1000+) for collision detection.
    void create_walls(std::string_view maze_string) noexcept
    {
        const auto* maze_data = maze_string.data();
        auto maze_len = maze_string.size();
        int current_row = 0;
        int current_col = 0;
        int wall_count = 0;

        for (size_t i = 0; i < maze_len; i++)
        {
            char c = maze_data[i];

            if (c == '\n')
            {
                current_col = 0;
                current_row++;
                continue;
            }

            // Only create bodies for wall characters
            if (c == static_cast<unsigned char>(mazes::barriers::SINGLE_SPACE))
            {
                current_col++;
                continue;;
            }

            // Calculate physics position (center of cell)
            float physX = (current_col * cell_size) / pixels_per_meter;
            float physY = (current_row * cell_size) / pixels_per_meter;

            // Pass nullptr for texture - walls render themselves and don't need individual texture ownership
            wall_bodies.emplace_back(physX, physY, 1000u + wall_count, nullptr, this->world_id);

            const auto& wall_id{wall_bodies.back().get_body_id()};

            // Shape properties
            b2ShapeDef shapeDef = b2DefaultShapeDef();
            shapeDef.density = 0.0f;

            // Create appropriate shape based on wall type
            if (c == static_cast<unsigned char>(mazes::barriers::HORIZONTAL))
            {
                // Horizontal wall: full width, thicker height for visibility
                const float halfWidth = (cell_size / pixels_per_meter) * 0.5f;
                const float halfHeight = (cell_size / pixels_per_meter) * 0.2f; // Increased from 0.1 to 0.2
                b2Polygon boxShape = b2MakeBox(halfWidth, halfHeight);
                b2CreatePolygonShape(wall_id, &shapeDef, &boxShape);
            }
            else if (c == static_cast<unsigned char>(mazes::barriers::VERTICAL))
            {
                // Vertical wall: thicker width for visibility, full height
                float halfWidth = (cell_size / pixels_per_meter) * 0.2f; // Increased from 0.1 to 0.2
                float halfHeight = (cell_size / pixels_per_meter) * 0.5f;
                b2Polygon boxShape = b2MakeBox(halfWidth, halfHeight);
                b2CreatePolygonShape(wall_id, &shapeDef, &boxShape);
            }
            else if (c == static_cast<unsigned char>(mazes::barriers::CORNER))
            {
                // Corner: square at intersection
                float halfSize = (cell_size / pixels_per_meter) * 0.2f; // Increased from 0.15 to 0.2
                b2Polygon boxShape = b2MakeBox(halfSize, halfSize);
                b2CreatePolygonShape(wall_id, &shapeDef, &boxShape);
            }
            else
            {
                // Fallback: create a small square for unrecognized characters
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Unknown wall character '%c' (0x%02X) at (%d, %d) - creating default shape",
                            c, static_cast<unsigned char>(c), current_col, current_row);
                float halfSize = (cell_size / pixels_per_meter) * 0.15f;
                b2Polygon boxShape = b2MakeBox(halfSize, halfSize);
                b2CreatePolygonShape(wall_id, &shapeDef, &boxShape);
            }

            current_col++;
            wall_count++;
        }

        SDL_Log("Created %d wall bodies", wall_count);
    }

    /// @brief Process Box2D collision events and handle game logic
    /// @details Iterates through contact events from Box2D and triggers:
    ///          - Wall collision handling (damage/destruction)
    ///          - Ball-to-ball collision handling
    ///          - Exit/goal detection
    ///          This should be called every physics step.
    void process_physics_collisions() const noexcept
    {
        if (B2_IS_NULL(world_id))
        {
            return;
        }

        // Get collision events from Box2D
        const b2ContactEvents contactEvents = b2World_GetContactEvents(world_id);

        // Handle hit events (impacts)
        for (int i = 0; i < contactEvents.hitCount; ++i)
        {
            b2ContactHitEvent* hitEvent = &contactEvents.hitEvents[i];
            b2BodyId bodyA = b2Shape_GetBody(hitEvent->shapeIdA);
            b2BodyId bodyB = b2Shape_GetBody(hitEvent->shapeIdB);

            // Check both orderings for wall collisions
            handle_wall_collision(bodyA, bodyB);
            handle_wall_collision(bodyB, bodyA);
        }
    }

    /// @brief Handle collision between a ball and a wall
    /// @param possibleWallId Body ID that might be a wall
    /// @param possibleBallId Body ID that might be a ball
    /// @details Checks user data to identify walls (ID 1000-1999) and applies:
    ///          - Damage to the wall (incrementing hit count)
    ///          - Dynamic collision response (impulses, spin)
    ///          - Wall destruction when hit threshold is reached
    ///          - Score increments
    static void handle_wall_collision(b2BodyId possibleWallId, b2BodyId possibleBallId) noexcept
    {
        if (B2_IS_NULL(possibleWallId) || B2_IS_NULL(possibleBallId))
        {
            return;
        }

        void* wallUserData = b2Body_GetUserData(possibleWallId);

        // Check if this is a wall (IDs 1000-1999)
        if (const auto wallValue = reinterpret_cast<uintptr_t>(wallUserData); wallValue >= 1000 && wallValue < 2000)
        {
            const int wallIndex = static_cast<int>(wallValue - 1000);

            // Get ball velocity to check impact speed
            b2Vec2 ballVel = b2Body_GetLinearVelocity(possibleBallId);
            float impactSpeed = b2Length(ballVel);

            // Only count significant impacts
            if (impactSpeed > 0.5f)
            {
                SDL_Log("Wall %d hit! Impact speed: %.2f", wallIndex, impactSpeed);

                // Apply impulse for dynamic collision response
                if (impactSpeed > 0)
                {
                    b2Vec2 normalizedVel = b2Normalize(ballVel);

                    // Add randomness to bounce
                    float randomAngle = ((rand() % 20) - 10) * 0.01f;
                    float cosR = cosf(randomAngle);
                    float sinR = sinf(randomAngle);
                    b2Vec2 adjustedDir = {
                        normalizedVel.x * cosR - normalizedVel.y * sinR,
                        normalizedVel.x * sinR + normalizedVel.y * cosR
                    };

                    // Apply opposing force
                    b2Vec2 opposingForce = b2MulSV(-0.7f * impactSpeed, adjustedDir);
                    b2Body_ApplyLinearImpulseToCenter(possibleBallId, opposingForce, true);

                    // Add spin for visual interest
                    float spin = (rand() % 10) * 0.3f;
                    b2Body_ApplyAngularImpulse(possibleBallId, spin, true);
                }
            }
        }
    }

    /// @brief Create a dynamic ball at the specified physics position
    /// @param phys_x X position in physics world (meters)
    /// @param phys_y Y position in physics world (meters)
    /// @param radius Ball radius in meters
    /// @return b2BodyId of the created ball
    void create_ball(float phys_x, float phys_y, float radius = 0.3f) noexcept
    {
        if (B2_IS_NULL(world_id))
        {
            return;
        }

        // Create ball object - pass nullptr for texture since balls render themselves
        // and don't need to own a texture
        ball_bodies.emplace_back(phys_x, phys_y, radius, nullptr, this->world_id);

        SDL_Log("Created ball at physics pos (%.2f, %.2f) with radius %.2f", phys_x, phys_y, radius);
    }

    /// @brief Create several test balls in the maze
    /// @details Places balls at various positions in the maze for testing
    void create_test_balls() noexcept
    {
        if (B2_IS_NULL(world_id) || max_cols == 0 || max_rows == 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Cannot create balls - world not initialized");
            return;
        }

        // Calculate world dimensions in meters
        const float worldWidth = (max_cols * cell_size) / pixels_per_meter;
        const float worldHeight = (max_rows * cell_size) / pixels_per_meter;

        // Create a few balls at different positions
        constexpr int numBalls = 3;
        for (int i = 0; i < numBalls; ++i)
        {
            // Place balls at even spacing across the top of the maze
            float x = worldWidth * (static_cast<float>(i) + 1.0f) / (numBalls + 1.0f);
            float y = worldHeight * 0.2f; // Near the top
            // Radius should be about 40% of a cell for good visibility
            create_ball(x, y, 0.4f);
        }

        SDL_Log("Created %d test balls", numBalls);
    }

    /// @brief Render all physics bodies in the world
    /// @details Renders tracked bodies (walls, balls, boundaries).
    ///          This provides a visual debug view of the physics simulation.
    void render_physics_world() noexcept
    {
        if (B2_IS_NULL(world_id))
        {
            return;
        }

        SDL_Renderer* renderer = sdl_helper.renderer;
        if (!renderer)
        {
            return;
        }

        // Draw background - first clear to light gray
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        SDL_RenderClear(renderer);

        // Render SDL logo in top-left corner if available
        if (sdl_logo && sdl_logo->get())
        {
            // Get logo dimensions
            const int logo_w = sdl_logo->get_width();
            const int logo_h = sdl_logo->get_height();

            // Position in top-left corner with small margin
            constexpr float margin = 10.0f;
            float logo_x = margin;
            float logo_y = margin;

            // Set proper blend mode to fix color issue
            SDL_SetTextureBlendMode(sdl_logo->get(), SDL_BLENDMODE_BLEND);

            // Set color mod to white (no tinting) - fixes light blue issue
            SDL_SetTextureColorMod(sdl_logo->get(), 255, 255, 255);

            // Draw the logo with some transparency so maze is still visible
            SDL_SetTextureAlphaMod(sdl_logo->get(), 128); // 50% transparency

            SDL_FRect dst_rect = {logo_x, logo_y, static_cast<float>(logo_w), static_cast<float>(logo_h)};
            SDL_RenderTexture(renderer, sdl_logo->get(), nullptr, &dst_rect);

            // Reset alpha for other rendering
            SDL_SetTextureAlphaMod(sdl_logo->get(), 255);
        }

        std::ranges::for_each(std::as_const(wall_bodies), [this](const wall& w)
        {
            w.draw(this->sdl_helper.renderer, this->pixels_per_meter, this->offset_x, this->offset_y);
        });

        // Draw balls with proper coordinate transformation
        std::ranges::for_each(std::as_const(ball_bodies), [this](const ball& b)
        {
            b.draw(this->sdl_helper.renderer, this->pixels_per_meter, this->offset_x, this->offset_y);
        });
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
    using std::ref;

    auto&& gamePtr = this->m_impl;

    auto previous = static_cast<double>(SDL_GetTicks());
    double accumulator = 0.0;

    SDL_Log("Entering game loop...\n");

    // Create maze physics
    gamePtr->create_physics_objects(mazes::create(mazes::configurator().rows(10).columns(10)), 10.0f);

    // Create some test balls to see the simulation
    gamePtr->create_test_balls();

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (gamePtr->window && gamePtr->window->is_open())
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

            gamePtr->update(static_cast<float>(FIXED_TIME_STEP) / 1000.f, std::ref(rng));
        }

        gamePtr->render(elapsed);
    }

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_END;
    emscripten_cancel_main_loop();
#endif

    return true;
}

