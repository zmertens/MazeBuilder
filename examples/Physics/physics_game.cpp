//
// physics_game class implementation
// Simple 2D physics simulation with bouncy balls that break walls
// Navigate from start to finish in a time-sensitive race
//

#include "physics_game.h"

#include <array>
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
#include "coordinates.h"
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

    sdl_helper sdl;

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
    b2BodyId boundary_body_id = b2_nullBodyId;
    b2WorldId world_id = b2_nullWorldId;
    // Scale factor for Box2D (which uses meters)
    float pixels_per_meter = 40.0f;

    std::function<std::string(mazes::randomizer&)> level_generator;
    bool level_loaded = false;

    // Body tracking for rendering
    std::vector<ball> ball_bodies;
    std::vector<wall> wall_bodies;

    physics_game_impl(std::string_view title, std::string_view version, int w, int h)
        : sdl{}
          , sdl_logo{std::make_unique<texture>()}
          , sfml_logo{std::make_unique<texture>()}
          , window{nullptr}
          , title{title}
          , version{version}
          , INIT_WINDOW_W{w}
          , INIT_WINDOW_H{h}
    {
        level_generator = [](mazes::randomizer& rng) -> std::string
        {
            return mazes::create(mazes::configurator()
                                .algo_id(rng(0, 1) == 0 ? mazes::algo::BINARY_TREE : mazes::algo::DFS)
                                .rows(rng(5, 25))
                                .columns(rng(5, 25)));
        };

        init_sdl();

        // Check if SDL initialization succeeded
        if (!sdl.window || !sdl.renderer)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create SDL window or renderer - cannot continue");
            // Don't initialize further objects if SDL failed
            return;
        }

        this->window = std::make_unique<render_window>(sdl.renderer, sdl.window);

        init_dear_imgui();

        init_textures();
    }

    ~physics_game_impl()
    {
        if (sdl.window || sdl.renderer)
        {
            sdl.destroy_and_quit();
        }
    }

    void init_sdl() noexcept
    {
        const auto window_title = title + " - " + version;
        this->sdl.init(window_title, INIT_WINDOW_W, INIT_WINDOW_H);
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
        ImGui_ImplSDL3_InitForSDLRenderer(this->sdl.window, this->sdl.renderer);
        ImGui_ImplSDLRenderer3_Init(this->sdl.renderer);

        ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(Cousine_Regular_compressed_data,
                                                             Cousine_Regular_compressed_size,
                                                             pixels_per_meter - 5.f);
    }

    void init_textures() const noexcept
    {
        if (!sdl_logo->loadFromFile(sdl.renderer, SDL_LOGO_FILEPATH))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load SDL logo texture from %s",
                         SDL_LOGO_FILEPATH.data());
        }

        if (!sfml_logo->loadFromFile(sdl.renderer, SFML_LOGO_FILEPATH))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load SFML logo texture from %s",
                         SFML_LOGO_FILEPATH.data());
        }

        if (SDL_Surface* bmp_surface = SDL_LoadBMP(WINDOW_ICON_FILEPATH.data()))
        {
            SDL_SetWindowIcon(sdl.window, bmp_surface);
            SDL_DestroySurface(bmp_surface);
        }
        else
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load window icon from %s: %s",
                         WINDOW_ICON_FILEPATH.data(), SDL_GetError());
        }
    }

    void processInput() noexcept
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

            if (level_loaded && event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_B)
            {
                level_loaded = false;
            }
        }
    }

    void update(const float dt, mazes::randomizer& rng, const int sub_steps = 4) noexcept
    {
        // Step the Box2D physics world if initialized
        if (B2_IS_NON_NULL(world_id))
        {
            // Step physics with substeps for stability
            // Substeps help with collision accuracy at high velocities
            b2World_Step(world_id, dt, sub_steps);

            // Process collision events and game logic
            process_physics_collisions(std::ref(rng));

            // Handle mouse dragging
            float mouse_x, mouse_y;
            const auto mouseState = SDL_GetMouseState(&mouse_x, &mouse_y);
            const auto [x, y] = screen_to_physics_coords(mouse_x, mouse_y,
                                                         offset_x, offset_y, pixels_per_meter);
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
            ImGui::Text("Press \'B\' to rebuild level");
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
        b2WorldDef world_def = b2DefaultWorldDef();
        world_def.gravity = {0.0f, 9.8f};
        world_id = b2CreateWorld(&world_def);

        // Set physics simulation parameters
        pixels_per_meter = 40.0f;

        // Clear body tracking
        wall_bodies.clear();
        ball_bodies.clear();
        boundary_body_id = b2_nullBodyId;

        SDL_Log("Physics world initialized with gravity: (0.0, 9.8)");
    }

    /// @brief Create maze physics objects from ASCII maze string
    /// @param maze_string ASCII representation of the maze ('+', '-', '|', ' ')
    /// @param cell_size_param Size of each cell in pixels
    /// @param rng
    /// @details Parses the maze string and creates Box2D static bodies for walls.
    ///          Walls are represented as:
    ///          - '+' : Corner joints (small squares)
    ///          - '-' : Horizontal walls
    ///          - '|' : Vertical walls
    ///          - ' ' : Empty path cells
    ///          Also creates boundary walls around the entire maze to contain balls.
    void create_physics_objects(const std::string_view maze_string, const float cell_size_param,
                                mazes::randomizer& rng) noexcept
    {
        if (maze_string.empty())
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Cannot create maze physics from empty string");
            return;
        }

        // Initialize or reset physics world
        init_physics();

        // Calculate maze dimensions by parsing the string
        const auto* maze_data = maze_string.data();
        const auto maze_len = maze_string.size();
        int current_row = 0;
        int col_count = 0;

        max_cols = 0;
        max_rows = 0;

        for (size_t i = 0; i < maze_len; i++)
        {
            if (maze_data[i] == '\n')
            {
                max_cols = std::max(max_cols, col_count);
                col_count = 0;
                current_row++;
            }
            else
            {
                col_count++;
            }
        }
        max_rows = current_row + 1;

        SDL_Log("Maze dimensions: %d rows x %d columns", max_rows, max_cols);

        // Store cell size and calculate centering offsets
        cell_size = cell_size_param;
        pixels_per_meter = cell_size; // Use cell size as the meter scale

        const auto maze_width = static_cast<float>(max_cols) * cell_size;
        const auto maze_height = static_cast<float>(max_rows) * cell_size;
        offset_x = std::max(0.0f, (static_cast<float>(INIT_WINDOW_W) - maze_width) / 2.0f);
        offset_y = std::max(0.0f, (static_cast<float>(INIT_WINDOW_H) - maze_height) / 2.0f);

        // Calculate world dimensions in physics units (meters)
        const auto world_width = (static_cast<float>(max_cols) * cell_size) / pixels_per_meter;
        const auto world_height = (static_cast<float>(max_rows) * cell_size) / pixels_per_meter;

        SDL_Log("Physics world size: %.2f x %.2f meters", world_width, world_height);
        SDL_Log("Cell size: %.2f pixels, pixelsPerMeter: %.2f", cell_size, pixels_per_meter);

        // Create boundary walls around the entire world
        create_world_boundaries(world_width, world_height);

        // Parse maze and create wall bodies
        create_walls(maze_string);

        // Create some test balls to see the simulation
        create_ball_bodies(std::ref(rng));
    }

    /// @brief Create static boundary walls around the physics world
    /// @param world_width Width of the world in meters
    /// @param world_height Height of the world in meters
    /// @details Creates 4 edge segments (top, bottom, left, right) to prevent
    ///          balls from escaping the play area. These are sensor boundaries.
    void create_world_boundaries(const float world_width, const float world_height) noexcept
    {
        // Create a static body for boundaries
        b2BodyDef boundary_def = b2DefaultBodyDef();
        boundary_def.type = b2_staticBody;
        boundary_def.userData = reinterpret_cast<void*>(3000);
        boundary_body_id = b2CreateBody(world_id, &boundary_def);

        // Shape properties for boundaries
        b2ShapeDef boundary_shape_def = b2DefaultShapeDef();
        boundary_shape_def.density = 0.0f;

        // Define boundary positions with a small margin
        constexpr float margin = 1.0f;
        constexpr float left = -margin;
        const float right = world_width + margin;
        constexpr float top = -margin;
        const float bottom = world_height + margin;

        // Top boundary
        const b2Segment top_seg = {{left, top}, {right, top}};
        b2CreateSegmentShape(boundary_body_id, &boundary_shape_def, &top_seg);

        // Bottom boundary
        const b2Segment bottom_seg = {{left, bottom}, {right, bottom}};
        b2CreateSegmentShape(boundary_body_id, &boundary_shape_def, &bottom_seg);

        // Left boundary
        const b2Segment left_seg = {{left, top}, {left, bottom}};
        b2CreateSegmentShape(boundary_body_id, &boundary_shape_def, &left_seg);

        // Right boundary
        const b2Segment right_seg = {{right, top}, {right, bottom}};
        b2CreateSegmentShape(boundary_body_id, &boundary_shape_def, &right_seg);

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
    void create_walls(const std::string_view maze_string) noexcept
    {
        auto maze_len = maze_string.size();
        int current_row = 0;
        int current_col = 0;
        int wall_count = 0;

        for (auto itr{maze_string.cbegin()}; itr != maze_string.cend(); ++itr, ++maze_len)
        {
            if (*itr == '\n')
            {
                current_col = 0;
                current_row++;
                continue;
            }

            // Only create bodies for wall characters
            if (*itr == static_cast<unsigned char>(mazes::barriers::SINGLE_SPACE))
            {
                current_col++;
                continue;;
            }

            // Calculate physics position (center of cell)
            float phys_x = (static_cast<float>(current_col) * cell_size) / pixels_per_meter;
            float phys_y = (static_cast<float>(current_row) * cell_size) / pixels_per_meter;

            // Pass nullptr for texture - walls render themselves and don't need individual texture ownership
            wall_bodies.emplace_back(phys_x, phys_y, 1000u + wall_count, this->world_id);

            const auto& wall_id{wall_bodies.back().get_body_id()};

            // Shape properties
            b2ShapeDef shapeDef = b2DefaultShapeDef();
            shapeDef.density = 0.0f;

            // Create appropriate shape based on wall type
            if (*itr == static_cast<unsigned char>(mazes::barriers::HORIZONTAL))
            {
                // Horizontal wall: full width, thicker height for visibility
                const auto half_width = (cell_size / pixels_per_meter) * 0.5f;
                const auto half_height = (cell_size / pixels_per_meter) * 0.2f;
                b2Polygon boxShape = b2MakeBox(half_width, half_height);
                b2CreatePolygonShape(wall_id, &shapeDef, &boxShape);
            }
            else if (*itr == static_cast<unsigned char>(mazes::barriers::VERTICAL))
            {
                // Vertical wall: thicker width for visibility, full height
                const auto half_width = (cell_size / pixels_per_meter) * 0.2f;
                const auto half_height = (cell_size / pixels_per_meter) * 0.5f;
                b2Polygon box_shape = b2MakeBox(half_width, half_height);
                b2CreatePolygonShape(wall_id, &shapeDef, &box_shape);
            }
            else if (*itr == static_cast<unsigned char>(mazes::barriers::CORNER))
            {
                // Corner: square at intersection
                const float half_size = (cell_size / pixels_per_meter) * 0.2f;
                b2Polygon boxShape = b2MakeBox(half_size, half_size);
                b2CreatePolygonShape(wall_id, &shapeDef, &boxShape);
            }
            else
            {
                // Fallback: create a small square for unrecognized characters
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Unknown wall character '%c' (0x%02X) at (%d, %d) - creating default shape",
                            *itr, static_cast<unsigned char>(*itr), current_col, current_row);
                const auto half_size = (cell_size / pixels_per_meter) * 0.15f;
                b2Polygon box_shape = b2MakeBox(half_size, half_size);
                b2CreatePolygonShape(wall_id, &shapeDef, &box_shape);
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
    void process_physics_collisions(mazes::randomizer& rng) const noexcept
    {
        if (B2_IS_NULL(world_id))
        {
            return;
        }

        // Get collision events from Box2D
        const b2ContactEvents contact_events = b2World_GetContactEvents(world_id);

        // Handle hit events (impacts)
        for (int i = 0; i < contact_events.hitCount; ++i)
        {
            const b2ContactHitEvent* hit_event = &contact_events.hitEvents[i];
            const b2BodyId body_a = b2Shape_GetBody(hit_event->shapeIdA);
            const b2BodyId body_b = b2Shape_GetBody(hit_event->shapeIdB);

            // Check both orderings for wall collisions
            handle_wall_collision(body_a, body_b, std::ref(rng));
            handle_wall_collision(body_b, body_a, std::ref(rng));
        }
    }

    /// @brief Handle collision between a ball and a wall
    /// @param possible_wall_id Body ID that might be a wall
    /// @param possible_ball_id Body ID that might be a ball
    /// @param rng
    /// @details Checks user data to identify walls (ID 1000-1999) and applies:
    ///          - Damage to the wall (incrementing hit count)
    ///          - Dynamic collision response (impulses, spin)
    ///          - Wall destruction when hit threshold is reached
    static void handle_wall_collision(b2BodyId possible_wall_id, b2BodyId possible_ball_id,
                                      mazes::randomizer& rng) noexcept
    {
        if (B2_IS_NULL(possible_wall_id) || B2_IS_NULL(possible_ball_id))
        {
            return;
        }

        void* wallUserData = b2Body_GetUserData(possible_wall_id);

        // Check if this is a wall (IDs 1000-1999)
        if (const auto wall_value = reinterpret_cast<uintptr_t>(wallUserData); wall_value >= 1000 && wall_value < 2000)
        {
            const int wall_index = static_cast<int>(wall_value - 1000);

            // Get ball velocity to check impact speed
            const b2Vec2 ball_velocity = b2Body_GetLinearVelocity(possible_ball_id);

            // Only count significant impacts
            if (const float impact_speed = b2Length(ball_velocity); impact_speed > 0.5f)
            {
                SDL_Log("Wall %d hit! Impact speed: %.2f", wall_index, impact_speed);

                // Apply impulse for dynamic collision response
                if (impact_speed > 0)
                {
                    const auto [x, y] = b2Normalize(ball_velocity);

                    // Add randomness to bounce
                    const auto random_angle = static_cast<float>(rng(10, 20)) * 0.01f;
                    const auto cos_r = SDL_cosf(random_angle);
                    const auto sin_r = SDL_sinf(random_angle);
                    b2Vec2 adjusted_dir = {
                        x * cos_r - y * sin_r,
                        x * sin_r + y * cos_r
                    };

                    // Apply opposing force
                    const b2Vec2 opposing_force = b2MulSV(-0.7f * impact_speed, adjusted_dir);
                    b2Body_ApplyLinearImpulseToCenter(possible_ball_id, opposing_force, true);

                    // Add spin for visual interest
                    const auto spin = static_cast<float>(rng(1, 10)) * 0.3f;
                    b2Body_ApplyAngularImpulse(possible_ball_id, spin, true);
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

        ball_bodies.emplace_back(phys_x, phys_y, radius, this->world_id);
    }

    /// @brief Create several test balls in the maze
    /// @details Places balls at various positions in the maze for testing
    void create_ball_bodies(mazes::randomizer& rng) noexcept
    {
        if (B2_IS_NULL(world_id) || max_cols == 0 || max_rows == 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Cannot create balls - world not initialized");
            return;
        }

        // Calculate world dimensions in meters
        const float world_width = (static_cast<float>(max_cols) * cell_size) / pixels_per_meter;
        const float world_height = (static_cast<float>(max_rows) * cell_size) / pixels_per_meter;

        // Create a few balls at different positions
        constexpr int num_balls = 100;
        for (int i = 0; i < num_balls; ++i)
        {
            // Place balls at even spacing across the top of the maze
            const float x = world_width * (static_cast<float>(i) + 1.0f) / (num_balls + 1.0f);
            const float y = world_height * 0.2f;
            // Radius should be about 40% of a cell for good visibility
            create_ball(x, y, static_cast<float>(rng(4, 7)) * 0.1f);
        }

        SDL_Log("Created %d test balls", num_balls);
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

        SDL_Renderer* renderer = sdl.renderer;
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
            const int logo_w = sdl_logo->get_width();
            const int logo_h = sdl_logo->get_height();

            // Position in top-left corner with small margin
            constexpr float margin = 10.0f;
            constexpr float logo_x = margin;
            constexpr float logo_y = margin;

            SDL_SetTextureBlendMode(sdl_logo->get(), SDL_BLENDMODE_BLEND);

            SDL_SetTextureColorMod(sdl_logo->get(), 255, 255, 255);

            // Draw the logo with some transparency so maze is still visible
            SDL_SetTextureAlphaMod(sdl_logo->get(), 128);

            const SDL_FRect dst_rect = {logo_x, logo_y, static_cast<float>(logo_w), static_cast<float>(logo_h)};
            SDL_RenderTexture(renderer, sdl_logo->get(), nullptr, &dst_rect);

            // Reset alpha for other rendering
            SDL_SetTextureAlphaMod(sdl_logo->get(), 255);
        }

        std::ranges::for_each(std::as_const(wall_bodies), [this](const wall& w)
        {
            w.draw(this->sdl.renderer, this->pixels_per_meter, this->offset_x, this->offset_y);
        });

        // Draw balls with proper coordinate transformation
        std::ranges::for_each(std::as_const(ball_bodies), [this](const ball& b)
        {
            b.draw(this->sdl.renderer, this->pixels_per_meter, this->offset_x, this->offset_y);
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
    auto&& gamePtr = this->m_impl;

    auto previous = static_cast<double>(SDL_GetTicks());
    double accumulator = 0.0;

    SDL_Log("Entering game loop...\n");

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

        if (!gamePtr->level_loaded)
        {
            gamePtr->create_physics_objects(gamePtr->level_generator(std::ref(rng)),
                static_cast<float>(rng(7, 12)),
                std::ref(rng));
            gamePtr->level_loaded = true;
        }
    }

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_END;
    emscripten_cancel_main_loop();
#endif

    return true;
}

