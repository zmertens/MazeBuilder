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
    static constexpr auto COMMON_RESOURCE_PATH_PREFIX = "resources";

    // Game-specific constants
    static constexpr float WALL_HIT_THRESHOLD = 4.0f;
    static constexpr float WALL_WIDTH = 0.1f;

    static constexpr int MAX_BALLS = 10;

    std::unique_ptr<render_window> window;

    sdl_helper sdlHelper;

    // FPS smoothing variables
    mutable double fpsUpdateTimer = 0.0;
    mutable int smoothedFps = 0;
    mutable float smoothedFrameTime = 0.0f;
    // Update display every 250ms
    static constexpr double FPS_UPDATE_INTERVAL = 250.0;

    const std::string title, version;
    const int INIT_WINDOW_W, INIT_WINDOW_H;

    // Maze rendering variables for physics coordinate system
    float cellSize = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    int maxCols = 0;
    int maxRows = 0;

    // Box2D world and physics components
    b2WorldId worldId = b2_nullWorldId;
    // Scale factor for Box2D (which uses meters)
    float pixelsPerMeter = 40.0f;

    // Game state
    int score = 0;

    // Body tracking for rendering
    std::vector<b2BodyId> wallBodies;
    std::vector<ball> ballBodies;
    b2BodyId boundaryBodyId = b2_nullBodyId;

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
        const auto windowTitle = title + " - " + version;
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

        ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(Cousine_Regular_compressed_data,
                                                             Cousine_Regular_compressed_size,
                                                             pixelsPerMeter);
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
        if (B2_IS_NON_NULL(worldId))
        {
            // Step physics with substeps for stability
            // Substeps help with collision accuracy at high velocities
            b2World_Step(worldId, dt, subSteps);

            // Process collision events and game logic
            processPhysicsCollisions();

            // Handle mouse dragging
            float mouseX, mouseY;
            const auto mouseState = SDL_GetMouseState(&mouseX, &mouseY);
            const auto mousePhysicsPos = screenToPhysics(mouseX, mouseY);
            const bool isMouseDown = (mouseState & SDL_BUTTON_LMASK) != 0;
            const mouse_states mice {
                isMouseDown ? mouse_states::button_state::DOWN : mouse_states::button_state::UP,
                mouse_states::button_state::UP,
                static_cast<int>(mousePhysicsPos.x),
                static_cast<int>(mousePhysicsPos.y)
            };

            for (auto& b : ballBodies)
            {
                b.update(dt, std::cref(mice), std::ref(rng));
            }
        }
    }

    void render(const double elapsed) noexcept
    {
        // Clear, draw, and present (like SFML)
        window->clear();
        window->beginFrame();

        // Render the physics world if initialized
        if (B2_IS_NON_NULL(worldId))
        {
            renderPhysicsWorld();
        }

#if defined(MAZE_DEBUG)
        // Window might be closed during draw calls/events
        if (window->isOpen())
        {
            this->handleFPS(elapsed);
        }
#endif

        window->display();
    }

    void handleFPS(const double elapsed) const noexcept
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
            ImGui::Text("FPS: %d", smoothedFps);
            ImGui::Text("Frame Time: %.2f ms", smoothedFrameTime);
            ImGui::End();
        }

        ImGui::PopFont();
    }

    /// @brief Initialize the Box2D physics world with gravity
    /// @details Creates a new Box2D world with downward gravity (9.8 m/s²) and
    ///          sets up simulation parameters for 60Hz physics updates.
    ///          This provides realistic physics for balls bouncing off walls.
    void initPhysics() noexcept
    {
        // Destroy existing world if present
        if (B2_IS_NON_NULL(worldId))
        {
            b2DestroyWorld(worldId);
        }

        // Create Box2D world with gravity pointing down
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {0.0f, 9.8f};
        worldId = b2CreateWorld(&worldDef);

        // Set physics simulation parameters
        pixelsPerMeter = 40.0f;

        // Clear body tracking
        wallBodies.clear();
        ballBodies.clear();
        boundaryBodyId = b2_nullBodyId;

        SDL_Log("Physics world initialized with gravity: (0.0, 9.8)");
    }

    /// @brief Convert screen coordinates to Box2D physics world coordinates
    /// @param screenX X coordinate in screen/pixel space
    /// @param screenY Y coordinate in screen/pixel space
    /// @return b2Vec2 position in physics world space (meters)
    /// @details This accounts for the maze offset and pixel-to-meter conversion.
    ///          Essential for mouse interaction with physics objects.
    b2Vec2 screenToPhysics(float screenX, float screenY) const noexcept
    {
        // Convert from screen coordinates to physics coordinates
        // by accounting for offset and scale
        const float physX = (screenX - offsetX) / pixelsPerMeter;
        const float physY = (screenY - offsetY) / pixelsPerMeter;

        return {physX, physY};
    }

    /// @brief Convert Box2D physics coordinates to screen coordinates
    /// @param physX X position in physics world (meters)
    /// @param physY Y position in physics world (meters)
    /// @return SDL_FPoint position in screen/pixel space
    /// @details Used for rendering physics objects at correct screen positions
    SDL_FPoint physicsToScreen(float physX, float physY) const noexcept
    {
        return {physX * pixelsPerMeter + offsetX, physY * pixelsPerMeter + offsetY};
    }

    /// @brief Create maze physics objects from ASCII maze string
    /// @param mazeString ASCII representation of the maze ('+', '-', '|', ' ')
    /// @param cellSize Size of each cell in pixels
    /// @details Parses the maze string and creates Box2D static bodies for walls.
    ///          Walls are represented as:
    ///          - '+' : Corner joints (small squares)
    ///          - '-' : Horizontal walls
    ///          - '|' : Vertical walls
    ///          - ' ' : Empty path cells
    ///          Also creates boundary walls around the entire maze to contain balls.
    void createMazePhysics(std::string_view mazeString, float cellSizeParam) noexcept
    {
        if (mazeString.empty())
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Cannot create maze physics from empty string");
            return;
        }

        // Initialize or reset physics world
        initPhysics();

        // Calculate maze dimensions by parsing the string
        const char* mazeData = mazeString.data();
        size_t mazeLen = mazeString.size();
        int currentRow = 0;
        int colCount = 0;

        maxCols = 0;
        maxRows = 0;

        for (size_t i = 0; i < mazeLen; i++)
        {
            if (mazeData[i] == '\n')
            {
                maxCols = std::max(maxCols, colCount);
                colCount = 0;
                currentRow++;
            }
            else
            {
                colCount++;
            }
        }
        maxRows = currentRow + 1;

        SDL_Log("Maze dimensions: %d rows x %d columns", maxRows, maxCols);

        // Store cell size and calculate centering offsets
        cellSize = cellSizeParam;
        pixelsPerMeter = cellSize; // Use cell size as the meter scale

        float mazeWidth = maxCols * cellSize;
        float mazeHeight = maxRows * cellSize;
        offsetX = std::max(0.0f, (INIT_WINDOW_W - mazeWidth) / 2.0f);
        offsetY = std::max(0.0f, (INIT_WINDOW_H - mazeHeight) / 2.0f);

        // Calculate world dimensions in physics units (meters)
        float worldWidth = (maxCols * cellSize) / pixelsPerMeter;
        float worldHeight = (maxRows * cellSize) / pixelsPerMeter;

        SDL_Log("Physics world size: %.2f x %.2f meters", worldWidth, worldHeight);
        SDL_Log("Cell size: %.2f pixels, pixelsPerMeter: %.2f", cellSize, pixelsPerMeter);

        // Create boundary walls around the entire world
        createWorldBoundaries(worldWidth, worldHeight);

        // Parse maze and create wall bodies
        createMazeWalls(mazeString);

        SDL_Log("Maze physics creation complete");
    }

    /// @brief Create static boundary walls around the physics world
    /// @param worldWidth Width of the world in meters
    /// @param worldHeight Height of the world in meters
    /// @details Creates 4 edge segments (top, bottom, left, right) to prevent
    ///          balls from escaping the play area. These are sensor boundaries.
    void createWorldBoundaries(float worldWidth, float worldHeight) noexcept
    {
        // Create a static body for boundaries
        b2BodyDef boundaryDef = b2DefaultBodyDef();
        boundaryDef.type = b2_staticBody;
        boundaryDef.userData = reinterpret_cast<void*>(3000); // Special ID for boundaries
        boundaryBodyId = b2CreateBody(worldId, &boundaryDef);

        // Shape properties for boundaries
        b2ShapeDef boundaryShapeDef = b2DefaultShapeDef();
        boundaryShapeDef.density = 0.0f;
        // Note: In Box2D 3.0, friction and restitution are set differently
        // They can be set on the shape after creation if needed

        // Define boundary positions with a small margin
        float margin = 1.0f;
        float left = -margin;
        float right = worldWidth + margin;
        float top = -margin;
        float bottom = worldHeight + margin;

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
    /// @param mazeString ASCII maze representation
    /// @details Creates static box-shaped bodies for each wall character.
    ///          - Horizontal walls ('-'): Full width, thin height
    ///          - Vertical walls ('|'): Thin width, full height
    ///          - Corners ('+'): Small square boxes
    ///          Walls are given unique user data IDs (1000+) for collision detection.
    void createMazeWalls(std::string_view mazeString) noexcept
    {
        const char* mazeData = mazeString.data();
        size_t mazeLen = mazeString.size();
        int currentRow = 0;
        int currentCol = 0;
        int wallCount = 0;

        for (size_t i = 0; i < mazeLen; i++)
        {
            char c = mazeData[i];

            if (c == '\n')
            {
                currentCol = 0;
                currentRow++;
                continue;
            }

            // Only create bodies for wall characters
            if (c == '-' || c == '|' || c == '+')
            {
                // Calculate physics position (center of cell)
                float physX = (currentCol * cellSize) / pixelsPerMeter;
                float physY = (currentRow * cellSize) / pixelsPerMeter;

                // Create static wall body
                b2BodyDef wallDef = b2DefaultBodyDef();
                wallDef.type = b2_staticBody;
                wallDef.position = {physX, physY};
                wallDef.userData = reinterpret_cast<void*>(1000 + wallCount);
                b2BodyId wallBodyId = b2CreateBody(worldId, &wallDef);

                // Wake up the body
                b2Body_SetAwake(wallBodyId, true);

                // Shape properties
                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.density = 0.0f;
                // Note: In Box2D 3.0, friction and restitution are set differently

                // Create appropriate shape based on wall type
                if (c == '-')
                {
                    // Horizontal wall: full width, thin height
                    const float halfWidth = (cellSize / pixelsPerMeter) * 0.5f;
                    const float halfHeight = (cellSize / pixelsPerMeter) * 0.1f;
                    b2Polygon boxShape = b2MakeBox(halfWidth, halfHeight);
                    b2CreatePolygonShape(wallBodyId, &shapeDef, &boxShape);
                }
                else if (c == '|')
                {
                    // Vertical wall: thin width, full height
                    float halfWidth = (cellSize / pixelsPerMeter) * 0.1f;
                    float halfHeight = (cellSize / pixelsPerMeter) * 0.5f;
                    b2Polygon boxShape = b2MakeBox(halfWidth, halfHeight);
                    b2CreatePolygonShape(wallBodyId, &shapeDef, &boxShape);
                }
                else if (c == '+')
                {
                    // Corner: small square
                    float halfSize = (cellSize / pixelsPerMeter) * 0.15f;
                    b2Polygon boxShape = b2MakeBox(halfSize, halfSize);
                    b2CreatePolygonShape(wallBodyId, &shapeDef, &boxShape);
                }

                // Store wall body for rendering
                wallBodies.push_back(wallBodyId);
                wallCount++;
            }

            currentCol++;
        }

        SDL_Log("Created %d wall bodies", wallCount);
    }

    /// @brief Process Box2D collision events and handle game logic
    /// @details Iterates through contact events from Box2D and triggers:
    ///          - Wall collision handling (damage/destruction)
    ///          - Ball-to-ball collision handling
    ///          - Exit/goal detection
    ///          This should be called every physics step.
    void processPhysicsCollisions() noexcept
    {
        if (B2_IS_NULL(worldId))
        {
            return;
        }

        // Get collision events from Box2D
        const b2ContactEvents contactEvents = b2World_GetContactEvents(worldId);

        // Handle hit events (impacts)
        for (int i = 0; i < contactEvents.hitCount; ++i)
        {
            b2ContactHitEvent* hitEvent = &contactEvents.hitEvents[i];
            b2BodyId bodyA = b2Shape_GetBody(hitEvent->shapeIdA);
            b2BodyId bodyB = b2Shape_GetBody(hitEvent->shapeIdB);

            // Check both orderings for wall collisions
            handleWallCollision(bodyA, bodyB);
            handleWallCollision(bodyB, bodyA);
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
    void handleWallCollision(b2BodyId possibleWallId, b2BodyId possibleBallId) noexcept
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

                // TODO: Increment wall hit count and check for destruction
                // This requires tracking wall entities separately
                score += 10;
            }
        }
    }

    /// @brief Create a dynamic ball at the specified physics position
    /// @param physX X position in physics world (meters)
    /// @param physY Y position in physics world (meters)
    /// @param radius Ball radius in meters
    /// @return b2BodyId of the created ball
    void createBall(float physX, float physY, float radius = 0.3f) noexcept
    {
        if (B2_IS_NULL(worldId))
        {
            return;
        }

        // Create dynamic body for the ball
        b2BodyDef ballDef = b2DefaultBodyDef();
        ballDef.type = b2_dynamicBody;
        ballDef.position = {physX, physY};
        ballDef.linearDamping = 0.1f;   // Small damping for realistic motion
        ballDef.angularDamping = 0.1f;
        b2BodyId ballBodyId = b2CreateBody(worldId, &ballDef);

        // Create circle shape
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;  // Standard density

        b2Circle circle = {{0.0f, 0.0f}, radius};
        b2CreateCircleShape(ballBodyId, &shapeDef, &circle);

        // Create ball object and assign the body ID
        ballBodies.emplace_back(physX, physY, radius, nullptr);
        ballBodies.back().setBodyId(ballBodyId);

        SDL_Log("Created ball at physics pos (%.2f, %.2f) with radius %.2f", physX, physY, radius);
    }

    /// @brief Create several test balls in the maze
    /// @details Places balls at various positions in the maze for testing
    void createTestBalls() noexcept
    {
        if (B2_IS_NULL(worldId) || maxCols == 0 || maxRows == 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Cannot create balls - world not initialized");
            return;
        }

        // Calculate world dimensions in meters
        const float worldWidth = (maxCols * cellSize) / pixelsPerMeter;
        const float worldHeight = (maxRows * cellSize) / pixelsPerMeter;

        // Create a few balls at different positions
        constexpr int numBalls = 3;
        for (int i = 0; i < numBalls; ++i)
        {
            // Place balls at even spacing across the top of the maze
            float x = worldWidth * (static_cast<float>(i) + 1.0f) / (numBalls + 1.0f);
            float y = worldHeight * 0.2f; // Near the top
            // Radius should be about 40% of a cell for good visibility
            createBall(x, y, 0.4f);
        }

        SDL_Log("Created %d test balls", numBalls);
    }

    /// @brief Render all physics bodies in the world
    /// @details Renders tracked bodies (walls, balls, boundaries).
    ///          This provides a visual debug view of the physics simulation.
    void renderPhysicsWorld() noexcept
    {
        if (B2_IS_NULL(worldId))
        {
            return;
        }

        SDL_Renderer* renderer = sdlHelper.renderer;
        if (!renderer)
        {
            return;
        }

        // Draw background
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        SDL_RenderClear(renderer);

        // Draw walls (black)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        for (const auto& wallId : wallBodies)
        {
            if (B2_IS_NULL(wallId))
            {
                continue;
            }

            b2Vec2 position = b2Body_GetPosition(wallId);
            b2Rot rotation = b2Body_GetRotation(wallId);

            // Get the shape from the body
            int shapeCount = b2Body_GetShapeCount(wallId);
            if (shapeCount == 0)
            {
                continue;
            }

            // Get first shape (walls only have one)
            b2ShapeId shapeIds[1];
            b2Body_GetShapes(wallId, shapeIds, 1);
            b2ShapeId shapeId = shapeIds[0];

            if (B2_IS_NULL(shapeId))
            {
                continue;
            }

            // Get polygon shape
            b2Polygon polygon = b2Shape_GetPolygon(shapeId);

            // Transform and render
            SDL_FPoint points[4];
            for (int i = 0; i < polygon.count && i < 4; ++i)
            {
                // Rotate vertex
                b2Vec2 rotated = b2RotateVector(rotation, polygon.vertices[i]);
                // Translate to position
                b2Vec2 world = b2Add(position, rotated);
                // Convert to screen
                points[i] = physicsToScreen(world.x, world.y);
            }

            // Draw filled rectangle
            if (polygon.count == 4)
            {
                float minX = std::min({points[0].x, points[1].x, points[2].x, points[3].x});
                float maxX = std::max({points[0].x, points[1].x, points[2].x, points[3].x});
                float minY = std::min({points[0].y, points[1].y, points[2].y, points[3].y});
                float maxY = std::max({points[0].y, points[1].y, points[2].y, points[3].y});

                SDL_FRect rect = {minX, minY, maxX - minX, maxY - minY};
                SDL_RenderFillRect(renderer, &rect);
            }
        }

      // Draw balls with proper coordinate transformation
      std::ranges::for_each(std::as_const(ballBodies), [this](const ball& b)
      {
          b.draw(this->sdlHelper.renderer, this->pixelsPerMeter, this->offsetX, this->offsetY);
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
    gamePtr->createMazePhysics(mazes::create(mazes::configurator().rows(10).columns(10)), 10.0f);

    // Create some test balls to see the simulation
    gamePtr->createTestBalls();

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

