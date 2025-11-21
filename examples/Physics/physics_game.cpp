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

#include <box2d/box2d.h>

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

    // Maze rendering variables for physics coordinate system
    float cellSize = 0.0f; // Size of a maze cell in pixels
    float offsetX = 0.0f; // X offset for centering the maze
    float offsetY = 0.0f; // Y offset for centering the maze
    int maxCols = 0; // Number of columns in the maze
    int maxRows = 0; // Number of rows in the maze

    // Box2D world and physics components
    b2WorldId worldId = b2_nullWorldId; // Box2D world identifier
    float timeStep = 1.0f / 60.0f; // Physics time step (60Hz)
    float pixelsPerMeter = 40.0f; // Scale factor for Box2D (which uses meters)

    // Game state
    int score = 0;

    // Mouse/touch interaction state
    bool isDragging = false;
    int draggedBallIndex = -1;
    b2Vec2 lastMousePos = {0.0f, 0.0f};

    // Body tracking for rendering
    std::vector<b2BodyId> wallBodies;
    std::vector<b2BodyId> ballBodies;
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

    void update(const float dt, int subSteps = 4) noexcept
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
            auto mouseState = SDL_GetMouseState(&mouseX, &mouseY);
            bool isMouseDown = (mouseState & SDL_BUTTON_LMASK) != 0;
            updateBallDrag(mouseX, mouseY, isMouseDown);
        }
    }

    void render(double& currentTimeStep, const double elapsed) noexcept
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
    }

    // ============================================================================
    // PHYSICS SYSTEM METHODS
    // ============================================================================

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
        worldDef.gravity = {0.0f, 9.8f}; // Gravity in meters/second²
        worldId = b2CreateWorld(&worldDef);

        // Set physics simulation parameters
        timeStep = 1.0f / 60.0f; // 60Hz simulation
        pixelsPerMeter = 40.0f; // Good scaling for visibility

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
        float physX = (screenX - offsetX) / pixelsPerMeter;
        float physY = (screenY - offsetY) / pixelsPerMeter;

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
                wallDef.userData = reinterpret_cast<void*>(1000 + wallCount); // Unique wall ID
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
                    float halfWidth = (cellSize / pixelsPerMeter) * 0.5f;
                    float halfHeight = (cellSize / pixelsPerMeter) * 0.1f;
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
        b2ContactEvents contactEvents = b2World_GetContactEvents(worldId);

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

        // Handle begin contact events (for sensors/triggers)
        for (int i = 0; i < contactEvents.beginCount; ++i)
        {
            b2ContactBeginTouchEvent* beginEvent = &contactEvents.beginEvents[i];
            b2BodyId bodyA = b2Shape_GetBody(beginEvent->shapeIdA);
            b2BodyId bodyB = b2Shape_GetBody(beginEvent->shapeIdB);

            // TODO: Handle goal/exit detection here
            // Check if a ball reached the exit using userData
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
        uintptr_t wallValue = reinterpret_cast<uintptr_t>(wallUserData);

        // Check if this is a wall (IDs 1000-1999)
        if (wallValue >= 1000 && wallValue < 2000)
        {
            int wallIndex = static_cast<int>(wallValue - 1000);

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

    /// @brief Handle mouse/touch dragging of physics balls
    /// @param mouseX Screen X coordinate of mouse/touch
    /// @param mouseY Screen Y coordinate of mouse/touch
    /// @param isMouseDown Whether mouse button is pressed
    /// @details Allows player to drag balls around the screen by:
    ///          - Detecting which ball is under the cursor
    ///          - Applying forces to move ball toward mouse position
    ///          - Setting velocity directly for responsive control
    ///          This creates a "mouse joint" effect without actual joints.
    void updateBallDrag(float mouseX, float mouseY, bool isMouseDown) noexcept
    {
        // Convert screen position to physics coordinates
        b2Vec2 mousePhysicsPos = screenToPhysics(mouseX, mouseY);

        if (isMouseDown)
        {
            if (!isDragging)
            {
                // Try to find a ball to start dragging - search from front to back
                for (int i = 0; i < static_cast<int>(ballBodies.size()); ++i)
                {
                    b2BodyId ballId = ballBodies[i];
                    if (B2_IS_NULL(ballId))
                    {
                        continue;
                    }

                    // Get ball position
                    b2Vec2 ballPos = b2Body_GetPosition(ballId);

                    // Get ball radius
                    int shapeCount = b2Body_GetShapeCount(ballId);
                    if (shapeCount == 0)
                    {
                        continue;
                    }

                    b2ShapeId shapeIds[1];
                    b2Body_GetShapes(ballId, shapeIds, 1);
                    b2ShapeId shapeId = shapeIds[0];

                    if (B2_IS_NULL(shapeId))
                    {
                        continue;
                    }

                    b2Circle circle = b2Shape_GetCircle(shapeId);
                    float radius = circle.radius;

                    // Calculate distance from mouse to ball center
                    float dx = mousePhysicsPos.x - ballPos.x;
                    float dy = mousePhysicsPos.y - ballPos.y;
                    float distance = sqrtf(dx * dx + dy * dy);

                    // Check if mouse is over this ball (with improved hit detection)
                    if (distance <= radius * 2.0f)
                    {
                        // Start dragging this ball
                        isDragging = true;
                        draggedBallIndex = i;
                        lastMousePos = mousePhysicsPos;

                        // Wake up the body to ensure it responds to forces
                        b2Body_SetAwake(ballId, true);

                        // Apply a small impulse to "pick up" the ball
                        b2Vec2 impulse = {0.0f, -0.5f};
                        b2Body_ApplyLinearImpulseToCenter(ballId, impulse, true);

                        SDL_Log("Ball %d selected for dragging at physics pos (%.2f, %.2f)",
                                i, ballPos.x, ballPos.y);
                        break;
                    }
                }
            }
            else if (draggedBallIndex >= 0 && draggedBallIndex < static_cast<int>(ballBodies.size()))
            {
                // Continue dragging the selected ball
                b2BodyId ballId = ballBodies[draggedBallIndex];

                if (B2_IS_NON_NULL(ballId))
                {
                    // Get ball position
                    b2Vec2 ballPos = b2Body_GetPosition(ballId);

                    // Calculate vector from ball to mouse
                    b2Vec2 toTarget = {
                        mousePhysicsPos.x - ballPos.x,
                        mousePhysicsPos.y - ballPos.y
                    };

                    // Apply strong force for responsive dragging
                    float forceScale = 220.0f;
                    b2Vec2 force = {toTarget.x * forceScale, toTarget.y * forceScale};
                    b2Body_ApplyForceToCenter(ballId, force, true);

                    // Set target velocity for more direct control
                    float speedFactor = 15.0f;
                    b2Vec2 targetVelocity = {toTarget.x * speedFactor, toTarget.y * speedFactor};

                    // Limit max velocity for stability
                    float maxSpeed = 25.0f;
                    float currentSpeed = b2Length(targetVelocity);
                    if (currentSpeed > maxSpeed)
                    {
                        targetVelocity.x = targetVelocity.x * (maxSpeed / currentSpeed);
                        targetVelocity.y = targetVelocity.y * (maxSpeed / currentSpeed);
                    }

                    b2Body_SetLinearVelocity(ballId, targetVelocity);

                    // Log dragging for debugging (less frequently)
                    static int logCounter = 0;
                    if (++logCounter % 30 == 0)
                    {
                        SDL_Log("Dragging ball %d: mouse=(%.2f,%.2f), ball=(%.2f,%.2f), force=(%.2f,%.2f)",
                                draggedBallIndex, mousePhysicsPos.x, mousePhysicsPos.y,
                                ballPos.x, ballPos.y, force.x, force.y);
                    }

                    // Store current position for next frame
                    lastMousePos = mousePhysicsPos;
                }
            }
        }
        else
        {
            // Mouse released - stop dragging
            if (isDragging && draggedBallIndex >= 0 && draggedBallIndex < static_cast<int>(ballBodies.size()))
            {
                b2BodyId ballId = ballBodies[draggedBallIndex];
                if (B2_IS_NON_NULL(ballId))
                {
                    // Apply release velocity based on recent movement
                    b2Vec2 currentVel = b2Body_GetLinearVelocity(ballId);
                    // Keep some of the current velocity for natural release feel
                    b2Vec2 releaseVel = {currentVel.x * 0.8f, currentVel.y * 0.8f};
                    b2Body_SetLinearVelocity(ballId, releaseVel);
                }

                SDL_Log("Released ball %d", draggedBallIndex);
            }

            isDragging = false;
            draggedBallIndex = -1;
        }
    }

    /// @brief Create a dynamic ball at the specified physics position
    /// @param physX X position in physics world (meters)
    /// @param physY Y position in physics world (meters)
    /// @param radius Ball radius in meters
    /// @return b2BodyId of the created ball
    b2BodyId createBall(float physX, float physY, float radius = 0.3f) noexcept
    {
        if (B2_IS_NULL(worldId))
        {
            return b2_nullBodyId;
        }

        // Create dynamic body for the ball
        b2BodyDef ballDef = b2DefaultBodyDef();
        ballDef.type = b2_dynamicBody;
        ballDef.position = {physX, physY};
        ballDef.linearDamping = 0.1f; // Small damping for realistic motion
        ballDef.angularDamping = 0.1f;
        b2BodyId ballId = b2CreateBody(worldId, &ballDef);

        // Create circle shape
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f; // Standard density

        b2Circle circle = {{0.0f, 0.0f}, radius};
        b2CreateCircleShape(ballId, &shapeDef, &circle);

        // Store ball body for rendering
        ballBodies.push_back(ballId);

        SDL_Log("Created ball at physics pos (%.2f, %.2f) with radius %.2f", physX, physY, radius);
        return ballId;
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
        float worldWidth = (maxCols * cellSize) / pixelsPerMeter;
        float worldHeight = (maxRows * cellSize) / pixelsPerMeter;

        // Create a few balls at different positions
        int numBalls = 3;
        for (int i = 0; i < numBalls; ++i)
        {
            // Place balls at even spacing across the top of the maze
            float x = worldWidth * (i + 1.0f) / (numBalls + 1.0f);
            float y = worldHeight * 0.2f; // Near the top
            createBall(x, y, 0.55f);
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

        // Draw balls (blue)
        SDL_SetRenderDrawColor(renderer, 0, 100, 255, 255);
        for (const auto& ballId : ballBodies)
        {
            if (B2_IS_NULL(ballId))
            {
                continue;
            }

            b2Vec2 position = b2Body_GetPosition(ballId);

            // Get the shape
            int shapeCount = b2Body_GetShapeCount(ballId);
            if (shapeCount == 0)
            {
                continue;
            }

            b2ShapeId shapeIds[1];
            b2Body_GetShapes(ballId, shapeIds, 1);
            b2ShapeId shapeId = shapeIds[0];

            if (B2_IS_NULL(shapeId))
            {
                continue;
            }

            // Get circle shape
            b2Circle circle = b2Shape_GetCircle(shapeId);
            b2Vec2 worldCenter = b2Add(position, circle.center);
            SDL_FPoint screenCenter = physicsToScreen(worldCenter.x, worldCenter.y);
            float screenRadius = circle.radius * pixelsPerMeter;

            // Draw filled circle using simple pixel filling
            int iRadius = static_cast<int>(screenRadius);
            for (int y = -iRadius; y <= iRadius; ++y)
            {
                for (int x = -iRadius; x <= iRadius; ++x)
                {
                    if (x * x + y * y <= iRadius * iRadius)
                    {
                        SDL_RenderPoint(renderer, screenCenter.x + x, screenCenter.y + y);
                    }
                }
            }
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

    // Create maze physics
    gamePtr->createMazePhysics(mazes::create(mazes::configurator().rows(10).columns(10)), 40.0f);

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

