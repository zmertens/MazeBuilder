//
// Physics class implementation
// Simple 2D physics simulation with bouncy balls using SDL3, box2d, and maze_builder
// 
// Threading technique uses 'islands':
// Example: https://github.com/SFML/SFML/tree/2.6.1/examples/island
//
// Audio Handling reference from SDL_AUDIO_STREAM: SDL\test\testaudio.c
//
// Score system: 10 points per wall destroyed
//              -1 point per friendly ball bounce
//              100 points per exit bounce
//

#include "Physics.hpp"

#include "cout_thread_safe.hpp"

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

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_opengl3.h>

#include <MazeBuilder/create.h>
#include <MazeBuilder/json_helper.h>

#include "AudioHelper.hpp"
#include "Ball.hpp"
#include "OrthographicCamera.hpp"
#include "SDLHelper.hpp"
#include "State.hpp"
#include "Texture.hpp"
#include "Wall.hpp"
#include "WorkerConcurrent.hpp"
#include "World.hpp"

static constexpr auto INIT_MAZE_ROWS = 10, INIT_MAZE_COLS = 10;
static const std::string RESOURCE_PATH_PREFIX = "resources";
static const std::string PHYSICS_JSON_PATH = RESOURCE_PATH_PREFIX + "/physics.json";

struct Physics::PhysicsImpl {
    
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

    SDLHelper sdlHelper;
    State state;
    WorkerConcurrent workerConcurrent;
    
    // Configuration loaded from physics.json
    std::unordered_map<std::string, std::string> resourceMap;
    
    // Splash screen texture
    SDL_Texture* splashTexture = nullptr;
    int splashWidth = 0, splashHeight = 0;

    // Camera for coordinate transformations
    std::unique_ptr<OrthographicCamera> camera;

    PhysicsImpl(const std::string& title, const std::string& version, int w, int h)
        : title{ title }, version{ version }, INIT_WINDOW_W{ w }, INIT_WINDOW_H{ h }
        , sdlHelper{}, state{ State::SPLASH }, workerConcurrent{state}
        , camera{ std::make_unique<OrthographicCamera>() } {

    }

    ~PhysicsImpl() {
        // Clean up splash texture
        if (splashTexture) {
            SDL_DestroyTexture(splashTexture);
            splashTexture = nullptr;
        }
        
        // Doing this here prevents issues when the app is launched and closed quickly
        workerConcurrent.generate("12345");
    }

    // Initialize the Box2D physics world
    void initPhysics() {
        // Create the world with gravity
        physicsWorld = std::make_unique<World>(9.8f);
        
        // Clear any existing entities
        walls.clear();
        balls.clear();
        
        // Set good values for physics simulation
        // Simulate at 60Hz
        timeStep = 1.0f / 60.0f;
         // Good scaling factor for visibility
        pixelsPerMeter = 40.0f;

        //auto s = std::format("{}", std::string("hi"));
    }
    
    // Load an image file using stb_image and create an SDL texture
    SDL_Texture* loadImageTexture(SDL_Renderer* renderer, const std::string& imagePath, int& width, int& height) {
        int channels;
        unsigned char* imageData = stbi_load(imagePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        
        if (!imageData) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load image: %s - %s\n", imagePath.c_str(), stbi_failure_reason());
            return nullptr;
        }
        
        // Create surface from image data (force RGBA format)
        SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA8888, imageData, width * 4);
        if (!surface) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create surface: %s\n", SDL_GetError());
            stbi_image_free(imageData);
            return nullptr;
        }
        
        // Create texture from surface
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create texture: %s\n", SDL_GetError());
        }
        
        // Clean up
        SDL_DestroySurface(surface);
        stbi_image_free(imageData);
        
        return texture;
    }
    
    // Convert screen coordinates to physics world coordinates
    b2Vec2 screenToPhysics(float screenX, float screenY) {
        // First account for camera transformation
        float worldX, worldY;
        int display_w, display_h;
        SDL_GetWindowSize(sdlHelper.window, &display_w, &display_h);
        camera->screenToWorld(screenX, screenY, worldX, worldY, display_w, display_h);
        
        // Then convert from world to physics coordinates by accounting for offset and scale
        float physX = (worldX - offsetX) / pixelsPerMeter;
        float physY = (worldY - offsetY) / pixelsPerMeter;
        
        return {physX, physY};
    }
    
    // Convert physics world coordinates to screen coordinates
    SDL_FPoint physicsToScreen(float physX, float physY) {
        return {physX * pixelsPerMeter, physY * pixelsPerMeter};
    }
    
    // Create a ball at the specified position
    Ball createBall(float x, float y) {
        static constexpr auto COMMON_BALL_RADIUS = 0.45f;
        // Pass 'this' as userData so we can identify balls in collision callbacks
        return Ball{ {x, y, 0.f}, COMMON_BALL_RADIUS, physicsWorld->getWorldId() };
    }

    // Convert the ASCII maze into Box2D physics objects
    void createMazePhysics(const std::string_view& mazeString, float cellSize) {
        // Clear any existing physics objects
        if (physicsWorld) {
            physicsWorld->destroyWorld();
        }
        
        // Create a new physics world
        initPhysics();
        walls.clear();
        balls.clear();
        
        // Calculate maze dimensions
        int maxCols = 0;
        int maxRows = 0;
        
        // First, calculate maze dimensions
        const char* mazeData = mazeString.data();
        size_t mazeLen = mazeString.size();
        int currentRow = 0;
        int colCount = 0;
        
        for (size_t i = 0; i < mazeLen; i++) {
            if (mazeData[i] == '\n') {
                maxCols = std::max(maxCols, colCount);
                colCount = 0;
                currentRow++;
            } else {
                colCount++;
            }
        }
        maxRows = currentRow + 1;
        
        SDL_Log("Maze dimensions: %d rows x %d columns", maxRows, maxCols);
        
        // Store the maze dimensions
        this->maxCols = maxCols;
        this->maxRows = maxRows;
        
        // Calculate cell dimensions based on the number of cells
        float cellWidth = cellSize;
        float cellHeight = cellSize;
        
        // Store cell size and calculate offsets for centering
        this->cellSize = cellSize;
        this->pixelsPerMeter = cellSize;
        
        // Center the maze in the display (calculate offsets)
        float mazeWidth = maxCols * cellSize;
        float mazeHeight = maxRows * cellSize;
        this->offsetX = (INIT_WINDOW_W - mazeWidth) / 2.0f;
        this->offsetY = (INIT_WINDOW_H - mazeHeight) / 2.0f;
        
        // Ensure offsets are never negative
        this->offsetX = std::max(0.0f, this->offsetX);
        this->offsetY = std::max(0.0f, this->offsetY);
        
        // Reset tracking variables
        currentRow = 0;
        int currentCol = 0;
        
        // Create world boundaries to prevent balls from escaping the screen
        float worldWidth = (maxCols * cellWidth) / pixelsPerMeter;
        float worldHeight = (maxRows * cellHeight) / pixelsPerMeter;
        
        SDL_Log("Physics world size: %.2f x %.2f meters", worldWidth, worldHeight);
        SDL_Log("Cell size: %.2f pixels, pixelsPerMeter: %.2f", cellSize, pixelsPerMeter);
        
        // Add boundary walls (top, bottom, left, right)
        b2BodyDef boundaryDef = b2DefaultBodyDef();
        boundaryDef.type = b2_staticBody;
        boundaryDef.userData = reinterpret_cast<void*>(3000); // Special identifier for boundaries
        b2BodyId boundaryBodyId = physicsWorld->createBody(&boundaryDef);
        
        b2ShapeDef boundaryShapeDef = b2DefaultShapeDef();
        boundaryShapeDef.density = 0.0f;
        boundaryShapeDef.material.friction = 0.3f;
        boundaryShapeDef.material.restitution = 0.8f;
        
        float worldLeft = -1.0f;
        float worldRight = worldWidth + 1.0f;
        float worldTop = -1.0f;
        float worldBottom = worldHeight + 1.0f;
        
        // Top boundary
        {
            b2Segment segment = {{worldLeft, worldTop}, {worldRight, worldTop}};
            physicsWorld->createSegmentShape(boundaryBodyId, &boundaryShapeDef, &segment);
        }
        
        // Bottom boundary
        {
            b2Segment segment = {{worldLeft, worldBottom}, {worldRight, worldBottom}};
            physicsWorld->createSegmentShape(boundaryBodyId, &boundaryShapeDef, &segment);
        }
        
        // Left boundary
        {
            b2Segment segment = {{worldLeft, worldTop}, {worldLeft, worldBottom}};
            physicsWorld->createSegmentShape(boundaryBodyId, &boundaryShapeDef, &segment);
        }
        
        // Right boundary
        {
            b2Segment segment = {{worldRight, worldTop}, {worldRight, worldBottom}};
            physicsWorld->createSegmentShape(boundaryBodyId, &boundaryShapeDef, &segment);
        }
        
        // Now create walls for maze structure using a more precise approach
        for (size_t i = 0; i < mazeLen; i++) {
            char c = mazeData[i];
            
            if (c == '\n') {
                currentCol = 0;
                currentRow++;
                continue;
            }
            
            // Convert to physics coordinates - position is center of the cell
            float physX = (currentCol * cellWidth) / pixelsPerMeter;
            float physY = (currentRow * cellHeight) / pixelsPerMeter;
            
            // Create walls for different maze characters
            if (c == '-' || c == '|' || c == '+') {
                b2BodyDef wallDef = b2DefaultBodyDef();
                wallDef.type = b2_staticBody;
                wallDef.position = {physX, physY};
                
                // Store the wall index in the user data to identify it later
                int wallIndex = walls.size();
                // We need to store our wall index in the userData
                wallDef.userData = reinterpret_cast<void*>(1000 + wallIndex); // Use offset to identify as wall
                
                b2BodyId wallBodyId = physicsWorld->createBody(&wallDef);
                
                // Explicitly set the body to be awake using the proper API function
                b2Body_SetAwake(wallBodyId, true);
                
                // Create shape definition with improved properties
                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.density = 0.0f;  // static body
                shapeDef.material.friction = 0.4f; // Higher friction
                shapeDef.material.restitution = 0.4f; // Moderate bounce
                shapeDef.material.rollingResistance = 1.0f; // Solid walls
                
                // Adjust wall size based on character
                if (c == '-') {
                    // Horizontal wall - width of full cell, height is smaller
                    float halfWidth = (cellWidth / pixelsPerMeter) * 0.5f;   // Full width of cell
                    float halfHeight = (cellHeight / pixelsPerMeter) * 0.1f; // 20% of cell height (centered)
                    
                    b2Polygon boxShape = b2MakeBox(halfWidth, halfHeight);
                    b2ShapeId wallShapeId = physicsWorld->createPolygonShape(wallBodyId, &shapeDef, &boxShape);
                    
                    // Add wall to our tracking
                    walls.push_back(Wall{wallBodyId, wallShapeId, 0, false, currentRow, currentCol, Wall::Orientation::HORIZONTAL });
                } 
                else if (c == '|') {
                    // Vertical wall - height of full cell, width is smaller
                    float halfWidth = (cellWidth / pixelsPerMeter) * 0.1f;   // 20% of cell width (centered)
                    float halfHeight = (cellHeight / pixelsPerMeter) * 0.5f; // Full height of cell
                    
                    b2Polygon boxShape = b2MakeBox(halfWidth, halfHeight);
                    b2ShapeId wallShapeId = physicsWorld->createPolygonShape(wallBodyId, &shapeDef, &boxShape);
                    
                    walls.push_back(Wall{ wallBodyId, wallShapeId, 0, false, currentRow, currentCol, Wall::Orientation::VERTICAL });
                }
                else if (c == '+') {
                    // Junction/corner - create a small square
                    float halfSize = (cellWidth / pixelsPerMeter) * 0.15f;  // 30% of cell size (squared)
                    
                    b2Polygon boxShape = b2MakeBox(halfSize, halfSize);
                    b2ShapeId wallShapeId = physicsWorld->createPolygonShape(wallBodyId, &shapeDef, &boxShape);
                    
                    // Add wall to our tracking
                    walls.push_back(Wall{ wallBodyId, wallShapeId, 0, false, currentRow, currentCol, Wall::Orientation::CORNER });
                }
            }
            
            currentCol++;
        }
        
        // Find all spaces where numbers (0-9) should be in the reference maze
        // These are typically at odd rows and columns (1-based indexing)
        std::vector<std::pair<int, int>> ballPositions;
        
        currentRow = 0;
        currentCol = 0;
        for (size_t i = 0; i < mazeLen; i++) {
            char c = mazeData[i];
            
            if (c == '\n') {
                currentCol = 0;
                currentRow++;
                continue;
            }
            
            // Look for empty spaces in paths - these are where balls should go
            // Corresponding to positions 0-9 in the reference maze
            if (c == ' ' && currentRow % 2 == 1 && currentCol % 2 == 1) {
                ballPositions.push_back({currentRow, currentCol});
            }
            
            currentCol++;
        }
        
        // Create the exit - place it at the top-left corner (to match cell 'P' in reference maze)
        // Find first open space in the maze
        int exitRow = -1, exitCol = -1;
        currentRow = 1;  // Start at row 1 (first path row)
        currentCol = 1;  // Start at col 1 (first path column)
        
        // If we found the top-left corner, use it for the exit
        if (!ballPositions.empty()) {
            exitRow = ballPositions[0].first;
            exitCol = ballPositions[0].second;
        } else {
            // Fallback to a reasonable position if we can't find spaces
            exitRow = 1;
            exitCol = 1;
        }
        
        // Create the exit at the found position
        exitCell.row = exitRow;
        exitCell.col = exitCol;
        
        b2BodyDef exitDef = b2DefaultBodyDef();
        exitDef.type = b2_staticBody;
        exitDef.position = {
            (exitCol * cellWidth) / pixelsPerMeter,
            (exitRow * cellHeight) / pixelsPerMeter
        };
        // Store special ID for exit
        exitDef.userData = reinterpret_cast<void*>(2000); // Use offset to identify as exit
        
        exitCell.bodyId = physicsWorld->createBody(&exitDef);
        
        // Create circle shape for exit
        b2ShapeDef circleDef = b2DefaultShapeDef();
        circleDef.density = 0.0f;  // static body
        circleDef.isSensor = true; // Make it a sensor so balls can pass through
        
        // Make exit circle size appropriate for the cell
        float exitRadius = cellWidth / (3.0f * pixelsPerMeter);
        b2Circle exitCircle = {{0.f, 0.f}, exitRadius};
        
        exitCell.shapeId = physicsWorld->createCircleShape(exitCell.bodyId, &circleDef, &exitCircle);
        
        // Create balls at the remaining positions (numbers 0-9 in reference maze)
        // Start from the end (highest number) and work backwards
        // Skip the first position as that's where the exit is
        int numBalls = std::min(10, (int)ballPositions.size() - 1);
        
        for (int i = 0; i < numBalls; i++) {
            // Get position from the end of the list (to match highest numbers first)
            int ballIndex = ballPositions.size() - 1 - i;
            if (ballIndex <= 0) break; // Skip the exit position
            
            auto [row, col] = ballPositions[ballIndex];
            
            float ballX = (col * cellWidth) / pixelsPerMeter;
            float ballY = (row * cellHeight) / pixelsPerMeter;
            
            balls.push_back(createBall(ballX, ballY));
            
            SDL_Log("Created ball at (%d,%d) position (%.2f,%.2f)",
                row, col, ballX, ballY);
        }
        
        SDL_Log("Maze physics created with %zu walls and %zu balls", walls.size(), balls.size());
        SDL_Log("Exit placed at row %d, col %d", exitRow, exitCol);
    }

    // Utility method for handling wall collisions
    void handleWallCollision(b2BodyId possibleWallId, b2BodyId possibleBallId) {
        // Check if possibleWallId is actually a wall by looking at userData tag
        void* wallUserData = b2Body_GetUserData(possibleWallId);
        void* ballUserData = b2Body_GetUserData(possibleBallId);
        
        // Check if it's a wall by checking the user data range
        // Walls are stored with pointer values starting at 1000
        uintptr_t wallValue = reinterpret_cast<uintptr_t>(wallUserData);
        
        // Debug output to help diagnose the issue
        SDL_Log("Wall collision check: wallValue=%zu, ballUserData=%p, this=%p", 
                wallValue, ballUserData, this);
        
        // Make sure this is a ball hitting a wall (ball userData is set to 'this')
        if (wallValue >= 1000 && wallValue < 2000) {
            // This is a wall - find its index
            int wallIndex = static_cast<int>(wallValue - 1000);
            
            // Make sure the index is valid
            if (wallIndex >= 0 && wallIndex < walls.size()) {
                Wall& wall = walls[wallIndex];
                
                if (wall.getIsDestroyed()) {
                    return; // Skip if wall is already flagged for destruction
                }
                
                // Get ball velocity to ensure only significant impacts count
                b2Vec2 ballVel = b2Body_GetLinearVelocity(possibleBallId);
                float impactSpeed = b2Length(ballVel);
                
                // Only count the hit if it's a significant impact
                if (impactSpeed > 0.5f) { // Lower threshold to make it easier to break walls
                    wall.setHitCount(wall.getHitCount() + 1);
                    SDL_Log("Wall hit! Wall index: %d, Hit count: %d/%d, Impact speed: %.2f", 
                           wallIndex, wall.getHitCount(), (int)WALL_HIT_THRESHOLD, impactSpeed);
                    
                    // Apply a small impulse to make the hit feel more impactful
                    b2Vec2 normalizedVel = ballVel;
                    if (impactSpeed > 0) {
                        normalizedVel = normalizedVel * (1.0f / impactSpeed);
                    }
                    // Add some randomness to the bounce for more dynamic behavior
                    float randomAngle = ((rand() % 20) - 10) * 0.01f; // Small random angle adjustment
                    float cosR = cosf(randomAngle);
                    float sinR = sinf(randomAngle);
                    b2Vec2 adjustedDir = {
                        normalizedVel.x * cosR - normalizedVel.y * sinR,
                        normalizedVel.x * sinR + normalizedVel.y * cosR
                    };
                    
                    // Create a more dynamic collision response
                    b2Vec2 opposingForce = adjustedDir * -0.7f * impactSpeed; // Scale with impact speed
                    b2Body_ApplyLinearImpulseToCenter(possibleBallId, opposingForce, true);
                    
                    // Increase the ball's angular velocity for more interesting physics
                    float spin = (rand() % 10) * 0.3f;
                    b2Body_ApplyAngularImpulse(possibleBallId, spin, true);
                }
                
                // Check if wall should break
                if (wall.getHitCount() >= WALL_HIT_THRESHOLD) {
                    wall.setIsDestroyed(true);
                    SDL_Log("Wall %d destroyed after %d hits!", wallIndex, wall.getHitCount());
                    
                    // Increment score
                    score += 10;
                }
            }
        }
    }
    
    // Utility method for handling ball-to-ball collisions
    void handleBallCollision(b2BodyId bodyAId, b2BodyId bodyBId) {
        // Check if both are balls (balls have user data pointer == this)
        auto* userDataA = b2Body_GetUserData(bodyAId);
        auto* userDataB = b2Body_GetUserData(bodyBId);
        
        if (userDataA == reinterpret_cast<void*>(this) && 
            userDataB == reinterpret_cast<void*>(this)) {
            
            // Find the corresponding ball objects
            Ball* ballA = nullptr;
            Ball* ballB = nullptr;

            for (Ball& ball : balls) {
                if (b2Body_GetPosition(bodyAId).x == b2Body_GetPosition(ball.getBodyId()).x
                && b2Body_GetPosition(bodyAId).y == b2Body_GetPosition(ball.getBodyId()).y ) {
                    ballA = &ball;
                }

                if (b2Body_GetPosition(bodyBId).x == b2Body_GetPosition(ball.getBodyId()).x
                && b2Body_GetPosition(bodyBId).y == b2Body_GetPosition(ball.getBodyId()).y ) {
                    ballB = &ball;
                }
            }
            
            if (ballA && ballB) {
                // Start explosion animation for both balls
                ballA->setIsExploding(true);
                ballB->setIsExploding(true);
            }
        }
    }

    // Handle ball dragging with improved interaction
    void updateBallDrag(float mouseX, float mouseY, bool isMouseDown) {
        // Convert screen coordinates to physics world coordinates
        b2Vec2 mousePhysicsPos = screenToPhysics(mouseX, mouseY);
        
        // If mouse button is pressed
        if (isMouseDown) {
            if (!isDragging) {
                // Try to find a ball to drag - search from front to back for better selection
                for (int i = 0; i < balls.size(); i++) {
                    auto& ball = balls[i];
                    
                    if (!ball.getIsActive() || ball.getIsExploding())
                        continue;
                    
                    // Get ball position and compare to mouse
                    b2Vec2 ballPos = b2Body_GetPosition(ball.getBodyId());
                    float distance = b2Distance(mousePhysicsPos, ballPos);
                    
                    // If mouse is over this ball with improved hit detection radius
                    if (distance <= ball.getRadius() * 2.0f) {
                        // Start dragging this ball
                        isDragging = true;
                        draggedBallIndex = i;
                        lastMousePos = mousePhysicsPos;
                        ball.setIsDragging(true);
                        
                        // Wake up the body explicitly to ensure it responds to forces
                        b2Body_SetAwake(ball.getBodyId(), true);
                        
                        // Apply a small impulse to "pick up" the ball
                        b2Vec2 impulse = {0.0f, -0.5f};
                        b2Body_ApplyLinearImpulseToCenter(ball.getBodyId(), impulse, true);
                        
                        SDL_Log("Ball %d selected for dragging at physics pos (%.2f, %.2f)", 
                                i, ballPos.x, ballPos.y);
                        break;
                    }
                }
            }
            else if (draggedBallIndex >= 0 && draggedBallIndex < balls.size()) {
                // Continue dragging the selected ball
                auto& ball = balls[draggedBallIndex];
                
                if (ball.getIsActive() && !ball.getIsExploding()) {
                    // Get ball position
                    b2Vec2 ballPos = b2Body_GetPosition(ball.getBodyId());
                    
                    // Calculate direct vector to target position
                    b2Vec2 toTarget = mousePhysicsPos - ballPos;
                    
                    // Use a mouse joint effect:
                    // 1. Apply stronger force for more responsive dragging
                    float forceScale = 220.0f;
                    b2Vec2 force = toTarget * forceScale;
                    
                    // Apply force to the ball center
                    b2Body_ApplyForceToCenter(ball.getBodyId(), force, true);
                    
                    // 2. Set a target velocity for more direct control
                    float speedFactor = 15.0f; // Increased for more responsiveness
                    b2Vec2 targetVelocity = toTarget * speedFactor;
                    
                    // Limit max velocity for stability
                    float maxSpeed = 25.0f; // Increased for better response
                    float currentSpeed = b2Length(targetVelocity);
                    if (currentSpeed > maxSpeed) {
                        targetVelocity = targetVelocity * (maxSpeed / currentSpeed);
                    }
                    
                    b2Body_SetLinearVelocity(ball.getBodyId(), targetVelocity);
                    
                    // Log dragging for debugging (less frequently)
                    static int logCounter = 0;
                    if (++logCounter % 30 == 0) {
                        SDL_Log("Dragging ball %d: mouse=(%.2f,%.2f), ball=(%.2f,%.2f), force=(%.2f,%.2f)", 
                            draggedBallIndex, mousePhysicsPos.x, mousePhysicsPos.y, 
                            ballPos.x, ballPos.y, force.x, force.y);
                    }
                    
                    // Store current position for next frame
                    lastMousePos = mousePhysicsPos;
                }
            }
        }
        else {
            // Mouse released, stop dragging
            if (isDragging && draggedBallIndex >= 0 && draggedBallIndex < balls.size()) {
                balls[draggedBallIndex].setIsDragging(false);
                
                // Apply a small release velocity based on recent movement
                auto& ball = balls[draggedBallIndex];
                if (ball.getIsActive() && !ball.getIsExploding()) {
                    b2Vec2 currentVel = b2Body_GetLinearVelocity(ball.getBodyId());
                    // Keep some of the current velocity for a natural release feel
                    b2Body_SetLinearVelocity(ball.getBodyId(), currentVel * 0.8f);
                }
                
                SDL_Log("Released ball %d", draggedBallIndex);
            }
            isDragging = false;
            draggedBallIndex = -1;
        }
    }

    // Draw a wall with appropriate color and damage effects based on hit count
    void drawWall(SDL_Renderer* renderer, const Wall& wall, float screenX, float screenY, 
                  float halfWidth, float halfHeight) const {
        // Calculate color based on hit count
        float hitRatio = static_cast<float>(wall.getHitCount()) / WALL_HIT_THRESHOLD;
        
        // Start with black and transition to yellow-orange-red as damage increases
        uint8_t red = 0, green = 0, blue = 0;
        
        if (wall.getHitCount() == 0) {
            // Undamaged wall - black
            red = green = blue = 0;
        } 
        else if (hitRatio < 0.33f) {
            // First stage - orange-ish with slight red
            red = 220;
            green = 120;
            blue = 0;
        }
        else if (hitRatio < 0.67f) {
            // Second stage - more red
            red = 240;
            green = 80;
            blue = 0;
        }
        else {
            // Final stage - bright red/yellow (near breaking)
            red = 255;
            green = static_cast<uint8_t>(hitRatio > 0.9f ? 255 : 40); // Flash yellow when about to break
            blue = 0;
        }
        
        SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
        
        // Draw the wall rectangle
        SDL_FRect rect = {
            screenX - halfWidth,
            screenY - halfHeight,
            halfWidth * 2,
            halfHeight * 2
        };
        
        SDL_RenderFillRect(renderer, &rect);
        
        // Add damage visual effects
        if (wall.getHitCount() > 0) {
            // Draw cracks that increase with damage
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180); // Whitish cracks
            
            int numCracks = 1 + static_cast<int>(hitRatio * 6);
            
            for (int i = 0; i < numCracks; i++) {
                // Generate random crack patterns
                float crackStartX = screenX - halfWidth * (0.8f * (static_cast<float>(rand()) / RAND_MAX));
                float crackStartY = screenY - halfHeight * (0.8f * (static_cast<float>(rand()) / RAND_MAX));
                
                // Create zigzag crack pattern
                float prevX = crackStartX;
                float prevY = crackStartY;
                
                int segments = 3 + static_cast<int>(hitRatio * 3);
                
                for (int j = 0; j < segments; j++) {
                    float nextX = screenX + halfWidth * (1.6f * (static_cast<float>(rand()) / RAND_MAX) - 0.8f);
                    float nextY = screenY + halfHeight * (1.6f * (static_cast<float>(rand()) / RAND_MAX) - 0.8f);
                    
                    SDL_RenderLine(renderer, prevX, prevY, nextX, nextY);
                    
                    prevX = nextX;
                    prevY = nextY;
                }
            }
            
            // Add pulsing effect when near breaking
            if (hitRatio > 0.75f) {
                static float pulseTimer = 0.0f;
                pulseTimer += 0.03f;
                
                float pulseAlpha = (sinf(pulseTimer * 10.0f) + 1.0f) * 0.5f * 150.0f; // Pulsing intensity
                
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, static_cast<uint8_t>(pulseAlpha));
                SDL_RenderRect(renderer, &rect);
                
                // Add a second rect for more intense effect when very close to breaking
                if (hitRatio > 0.9f) {
                    SDL_FRect innerRect = {
                        screenX - halfWidth + 2,
                        screenY - halfHeight + 2,
                        (halfWidth * 2) - 4,
                        (halfHeight * 2) - 4
                    };
                    SDL_RenderRect(renderer, &innerRect);
                }
            }
        }
    }
};

Physics::Physics(const std::string& title, const std::string& version, int w, int h)
    : m_impl{ std::make_unique<PhysicsImpl>(std::cref(title), std::cref(version), w, h)} {
}

Physics::~Physics() = default;

bool Physics::run() const noexcept {
    using namespace std;

    auto&& sdlHelper = this->m_impl->sdlHelper;
    auto&& workers = this->m_impl->workerConcurrent;

    sdlHelper.init();
    
    string_view titleView = this->m_impl->title;
    sdlHelper.window = SDL_CreateWindow(titleView.data(), this->m_impl->INIT_WINDOW_W, this->m_impl->INIT_WINDOW_H, SDL_WINDOW_RESIZABLE);
    if (!sdlHelper.window) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }
    sdlHelper.renderer = SDL_CreateRenderer(sdlHelper.window, nullptr);
    if (!sdlHelper.renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(sdlHelper.window);
        return false;
    }
    if (auto props = SDL_GetRendererProperties(sdlHelper.renderer); props != 0) {
        SDL_Log("Renderer created: %s\n", SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "default"));
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get renderer info: %s\n", SDL_GetError());
    }

    // Workers
    workers.initThreads();

    // Load physics.json configuration
    mazes::json_helper jh{};
    if (!jh.load(PHYSICS_JSON_PATH, this->m_impl->resourceMap)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load physics.json from: %s\n", PHYSICS_JSON_PATH);
        return false;
    }
    
    SDL_Log("Successfully loaded physics.json with %zu entries\n", this->m_impl->resourceMap.size());
    
    // Log loaded configuration for debugging
    for (const auto& [key, value] : this->m_impl->resourceMap) {
        SDL_Log("Config: %s = %s\n", key.c_str(), value.c_str());
    }

    // Load audio resources using configuration
    // Extract the actual filename from JSON string format (remove quotes and array brackets)
    auto extractValue = [](const std::string& jsonStr) -> std::string {
        // Handle array format like ["filename"]
        if (jsonStr.front() == '[' && jsonStr.back() == ']') {
            // Extract first element from array
            size_t start = jsonStr.find('"');
            size_t end = jsonStr.find('"', start + 1);
            if (start != std::string::npos && end != std::string::npos) {
                return jsonStr.substr(start + 1, end - start - 1);
            }
        }
        // Handle simple string format with quotes
        if (jsonStr.size() >= 2 && jsonStr.front() == '"' && jsonStr.back() == '"') {
            return jsonStr.substr(1, jsonStr.size() - 2);
        }
        return jsonStr;
    };
    
    const auto generateOggFile = extractValue(this->m_impl->resourceMap["music_ogg"]);
    sf::SoundBuffer generateSoundBuffer;
    const auto generatePath = std::string(RESOURCE_PATH_PREFIX) + "/" + generateOggFile;
    if (!generateSoundBuffer.loadFromFile(generatePath)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load sound file: %s\n", generatePath.c_str());
        return false;
    } else {
        SDL_Log("Success loading sound file: %s\n", generatePath.c_str());
    }

    unique_ptr<sf::Sound> generateSound = make_unique<sf::Sound>(generateSoundBuffer);
    AudioHelper audioHelper{cref(generateSound)};

    // Load and set window icon from configuration
    const auto iconImageFile = extractValue(this->m_impl->resourceMap["icon_image"]);
    const auto iconPath = std::string(RESOURCE_PATH_PREFIX) + "/" + iconImageFile;
    SDL_Surface* icon = SDL_LoadBMP(iconPath.c_str());
    if (icon) {
        SDL_SetWindowIcon(sdlHelper.window, icon);
        SDL_DestroySurface(icon);
        SDL_Log("Successfully loaded icon: %s\n", iconPath.c_str());
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load icon: %s - %s\n", iconPath.c_str(), SDL_GetError());
    }

    // Load WAV file from configuration
    const auto loadingWavFile = extractValue(this->m_impl->resourceMap["music_wav"]);
    const auto loadingPath = std::string(RESOURCE_PATH_PREFIX) + "/" + loadingWavFile;
    if (!sdlHelper.loadWAV(loadingPath.c_str())) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load WAV file: %s\n", loadingPath.c_str());
    } else {
        SDL_Log("Successfully loaded WAV file: %s\n", loadingPath.c_str());
    }

    sdlHelper.playAudioStream();
    
    // Load splash image from configuration
    const auto splashImageFile = extractValue(this->m_impl->resourceMap["splash_image"]);
    const auto splashImagePath = std::string(RESOURCE_PATH_PREFIX) + "/" + splashImageFile;
    this->m_impl->splashTexture = this->m_impl->loadImageTexture(sdlHelper.renderer, splashImagePath, 
                                                                 this->m_impl->splashWidth, this->m_impl->splashHeight);
    if (this->m_impl->splashTexture) {
        SDL_Log("Successfully loaded splash image: %s (%dx%d)\n", splashImagePath.c_str(), 
                this->m_impl->splashWidth, this->m_impl->splashHeight);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load splash image: %s\n", splashImagePath.c_str());
    }
    
    auto&& renderer = sdlHelper.renderer;
    SDL_SetRenderVSync(renderer, true);
    auto&& window = sdlHelper.window;

    vector<SDL_Vertex> level;
    
    // Create a static persistent string to store the maze data
    static std::string persistentMazeStr;
    
    auto&& gState = this->m_impl->state;
    gState = State::PLAY;
    
    // Set a good default value for pixelsPerMeter
    this->m_impl->pixelsPerMeter = 20.0f;
    
    // Generate initial level at startup
    int display_w, display_h;
    SDL_GetWindowSize(window, &display_w, &display_h);
    this->generateNewLevel(ref(persistentMazeStr), display_w, display_h);
    
    // Log physics world state after level generation
    SDL_Log("Physics world created, num walls: %zu, num balls: %zu", 
        this->m_impl->walls.size(), this->m_impl->balls.size());
    
    // Initial physics simulation step to ensure bodies are positioned
    if (this->m_impl->physicsWorld) {
        SDL_Log("Performing initial physics step");
        this->m_impl->physicsWorld->step(this->m_impl->timeStep, 4);
    }

    double previous = static_cast<double>(SDL_GetTicks());
    double accumulator = 0.0, currentTimeStep = 0.0;
    while (gState != State::DONE) {
        static constexpr auto FIXED_TIME_STEP = 1.0 / 60.0;
        auto elapsed = static_cast<double>(SDL_GetTicks()) - previous;
        previous = static_cast<double>(SDL_GetTicks());
        accumulator += elapsed;
        
        // Handle events and update physics at a fixed time step
        while (accumulator >= FIXED_TIME_STEP) {
            sdlHelper.poll_events(ref(gState), ref(this->m_impl->camera));
            accumulator -= FIXED_TIME_STEP;
            currentTimeStep += FIXED_TIME_STEP;
        }

        // Update physics simulation if we're in PLAY state
        if (gState == State::PLAY && this->m_impl->physicsWorld) {
             
            // Step Box2D world with sub-steps instead of velocity/position iterations
            // Using 4 sub-steps as recommended in the migration guide
            this->m_impl->physicsWorld->step(FIXED_TIME_STEP, 4);
             
            // Handle collisions and physics interactions
            this->processPhysicsCollisions();
            this->updatePhysicsObjects();
        }

        sdlHelper.updateAudioData();
        
        // Get window dimensions
        SDL_GetWindowSize(window, &display_w, &display_h);
        
        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // Light gray background
        SDL_RenderClear(renderer);
        
        // Render splash screen if in SPLASH state
        if (gState == State::SPLASH && this->m_impl->splashTexture) {
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
        
        // Generate new level if needed
        if (gState == State::UPLOADING_LEVEL) {
            audioHelper.playSound("generate");
            this->m_impl->workerConcurrent.generate("things and stuff");
            this->generateNewLevel(ref(persistentMazeStr), display_w, display_h);
            gState = State::PLAY;
        }
        
        // Draw the maze using the persistent maze string
        this->drawMaze(renderer, persistentMazeStr, display_w, display_h);
        
        // Draw the physics entities (balls, walls, exit) if we're in PLAY state
        if (gState == State::PLAY && 
            this->m_impl->physicsWorld) {
            this->drawPhysicsObjects(renderer);
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

// Process collisions in the Box2D world
void Physics::processPhysicsCollisions() const {
    // Process contact events in Box2D 3.1.0 style
    b2ContactEvents contactEvents = this->m_impl->physicsWorld->getContactEvents();
    
    // Handle contact hit events
    for (int i = 0; i < contactEvents.hitCount; ++i) {
        b2ContactHitEvent* hitEvent = &contactEvents.hitEvents[i];
        b2BodyId bodyA = b2Shape_GetBody(hitEvent->shapeIdA);
        b2BodyId bodyB = b2Shape_GetBody(hitEvent->shapeIdB);
        
        // Process wall collisions
        this->m_impl->handleWallCollision(bodyA, bodyB);
        this->m_impl->handleWallCollision(bodyB, bodyA);
        
        // Process ball-to-ball collisions
        this->m_impl->handleBallCollision(bodyA, bodyB);
    }
    
    // Handle contact begin events
    for (int i = 0; i < contactEvents.beginCount; ++i) {
        b2ContactBeginTouchEvent* beginEvent = &contactEvents.beginEvents[i];
        b2BodyId bodyA = b2Shape_GetBody(beginEvent->shapeIdA);
        b2BodyId bodyB = b2Shape_GetBody(beginEvent->shapeIdB);
        
        // Check if a ball reached the exit
        uintptr_t exitId = 2000;  // Special ID for exit
        
        if (reinterpret_cast<uintptr_t>(b2Body_GetUserData(bodyA)) == exitId || 
            reinterpret_cast<uintptr_t>(b2Body_GetUserData(bodyB)) == exitId) {
            
            b2BodyId ballId = reinterpret_cast<uintptr_t>(b2Body_GetUserData(bodyA)) == exitId ? bodyB : bodyA;
            
            // Find the ball that reached the exit
            for (auto& ball : this->m_impl->balls) {
                if (b2Body_GetPosition(ball.getBodyId()).x == b2Body_GetPosition(ballId).x
                && b2Body_GetPosition(ball.getBodyId()).y == b2Body_GetPosition(ballId).y &&
                ball.getIsActive() && !ball.getIsExploding()) {
                    // Collect the ball
                    ball.setIsActive(false);
                    this->m_impl->exitCell.ballsCollected++;
                    this->m_impl->score += 100;
                    
                    // Remove the ball
                    this->m_impl->physicsWorld->destroyBody(ball.getBodyId());
                    break;
                }
            }
        }
    }
}

// Update physics objects state (balls, walls)
void Physics::updatePhysicsObjects() const {
    // Handle any destroyed walls 
    for (int i = this->m_impl->walls.size() - 1; i >= 0; --i) {
        auto& wall = this->m_impl->walls[i];
        if (wall.getIsDestroyed()) {
            // Log destruction for debugging
            SDL_Log("Destroying wall %d with hit count %d", i, wall.getHitCount());
            
            // Destroy the body in the physics world
            if (B2_IS_NON_NULL(wall.getBodyId())) {
                this->m_impl->physicsWorld->destroyBody(wall.getBodyId());
                wall.setBodyId(b2_nullBodyId);
            }
            
            // Remove from our tracking array
            this->m_impl->walls.erase(this->m_impl->walls.begin() + i);
        }
    }
    
    // Handle exploding balls
    for (int i = this->m_impl->balls.size() - 1; i >= 0; --i) {
        auto& ball = this->m_impl->balls[i];
        
        // Update explosion animation
        if (ball.getIsExploding()) {
            ball.setExplosionTimer(ball.getExplosionTimer() + this->m_impl->timeStep);
            
            if (ball.getExplosionTimer() > 0.5f) { // After half a second, remove the ball
                if (B2_IS_NON_NULL(ball.getBodyId())) {
                    this->m_impl->physicsWorld->destroyBody(ball.getBodyId());
                    ball.setBodyId(b2_nullBodyId);
                }
                this->m_impl->balls.erase(this->m_impl->balls.begin() + i);
            }
        }
        
        // Check if ball is outside the play area
        if (ball.getIsActive() && !ball.getIsExploding()) {
            b2Vec2 position = b2Body_GetPosition(ball.getBodyId());
            
            // Boundary check with revised bounds - the previous values were too restrictive
            float worldWidth = (this->m_impl->maxCols * this->m_impl->cellSize) / this->m_impl->pixelsPerMeter;
            float worldHeight = (this->m_impl->maxRows * this->m_impl->cellSize) / this->m_impl->pixelsPerMeter;
            
            // Use more reasonable bounds based on actual maze dimensions plus margin
            float margin = 10.0f;  // More forgiving margin
            if (position.x < -margin || position.x > worldWidth + margin || 
                position.y < -margin || position.y > worldHeight + margin) {
                
                SDL_Log("Ball %d marked inactive - out of bounds at (%.2f, %.2f)", i, position.x, position.y);
                ball.setIsActive(false);
            }
        }
    }
    
    // Handle ball dragging
    float mouseX, mouseY;
    uint32_t mouseState = SDL_GetMouseState(&mouseX, &mouseY);
    bool isMouseDown = mouseState & SDL_BUTTON_LMASK;
    this->m_impl->updateBallDrag(mouseX, mouseY, isMouseDown);
}

// Draw the physics objects (balls, walls, exit)
void Physics::drawPhysicsObjects(SDL_Renderer* renderer) const {
    // Get values from impl
    float cellSize = this->m_impl->cellSize;
    float offsetX = this->m_impl->offsetX;
    float offsetY = this->m_impl->offsetY;
    int display_w, display_h;
    SDL_GetCurrentRenderOutputSize(renderer, &display_w, &display_h);
    
    // Get camera for transformations
    const auto& camera = this->m_impl->camera;
    
    // Count active balls for debugging
    int activeBallCount = 0;
    for (const auto& ball : this->m_impl->balls) {
        if (ball.getIsActive()) activeBallCount++;
    }
    
    // DEBUG: Draw a visible boundary around the physics world for debugging
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
    
    // Transform world bounds with camera
    SDL_FPoint topLeft = camera->worldToScreen(offsetX, offsetY, display_w, display_h);
    SDL_FPoint bottomRight = camera->worldToScreen(
        offsetX + this->m_impl->maxCols * cellSize, 
        offsetY + this->m_impl->maxRows * cellSize,
        display_w, display_h);
    
    SDL_FRect worldBounds = {
        topLeft.x, 
        topLeft.y, 
        bottomRight.x - topLeft.x,
        bottomRight.y - topLeft.y
    };
    SDL_RenderRect(renderer, &worldBounds);
    
    // Render walls
    for (const auto& wall : this->m_impl->walls) {
        if (wall.getIsDestroyed()) {

            continue;
        }
        
        wall.draw(renderer, camera, m_impl->pixelsPerMeter, offsetX, offsetY, cellSize, display_w, display_h);
    }
    
    // Render balls
    for (const auto& ball : this->m_impl->balls) {
        if (!ball.getIsActive()) {

            continue;
        }

        ball.draw(renderer, cref(camera), m_impl->pixelsPerMeter, offsetX, offsetY, cellSize, display_w, display_h);
    }
    
    // Render exit cell
    if (B2_IS_NON_NULL(this->m_impl->exitCell.bodyId)) {
        // Calculate world position
        float worldX = offsetX + (this->m_impl->exitCell.col * cellSize);
        float worldY = offsetY + (this->m_impl->exitCell.row * cellSize);
        
        // Apply camera transform
        SDL_FPoint screenPos = camera->worldToScreen(worldX, worldY, display_w, display_h);
        float screenX = screenPos.x;
        float screenY = screenPos.y;
        
        // Scale radius based on zoom
        float radius = cellSize * 0.4f * camera->zoom;
        
        // Use a bright color for the exit
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Bright green
        
        // Fill the exit circle
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius) {
                    SDL_RenderPoint(renderer, screenX + x, screenY + y);
                }
            }
        }
        
        // Draw X inside the exit with a contrasting color
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black
        SDL_RenderLine(renderer, 
            screenX - radius/2, screenY - radius/2,
            screenX + radius/2, screenY + radius/2);
        SDL_RenderLine(renderer, 
            screenX + radius/2, screenY - radius/2,
            screenX - radius/2, screenY + radius/2);
            
        // Log exit position
        // SDL_Log("Exit at screen pos (%.2f, %.2f)", screenX, screenY);
    }
}

// Draw the maze background
void Physics::drawMaze(SDL_Renderer* renderer, const std::string_view& cells, int display_w, int display_h) const {
    if (cells.empty()) {
        SDL_Log("ERROR: Empty maze data provided for drawing.\n");
        return;
    }
    
    // Get camera state for transformations
    const auto& camera = this->m_impl->camera;
    
    // Calculate maze dimensions
    int maxCols = 0;
    int maxRows = 0;
    const char* mazeData = cells.data();
    size_t mazeLen = cells.size();
    int currentRow = 0;
    int colCount = 0;
    
    for (size_t i = 0; i < mazeLen; i++) {
        if (mazeData[i] == '\n') {
            maxCols = std::max(maxCols, colCount);
            colCount = 0;
            currentRow++;
        } else {
            colCount++;
        }
    }
    maxRows = currentRow + 1;
    
    // Reset for drawing
    currentRow = 0;
    int currentCol = 0;
    
    // Calculate cell size to fit the display with proper padding
    float cellW = static_cast<float>(display_w) / static_cast<float>(maxCols);
    float cellH = static_cast<float>(display_h) / static_cast<float>(maxRows);
    float cellSize = std::min(cellW, cellH) * 0.95f; // 5% padding for better visibility
    
    // Make sure path cells are large enough to fit balls
    float minCellSize = 20.0f; // Minimum size for comfortable navigation
    cellSize = std::max(cellSize, minCellSize);
    
    // Center the maze in the display
    float mazeWidth = maxCols * cellSize;
    float mazeHeight = maxRows * cellSize;
    float offsetX = (display_w - mazeWidth) / 2.0f;
    float offsetY = (display_h - mazeHeight) / 2.0f;
    
    // Ensure the maze is centered even if it's bigger than the display
    offsetX = std::max(0.0f, offsetX);
    offsetY = std::max(0.0f, offsetY);
    
    // Store these values in the implementation for physics objects to use
    this->m_impl->cellSize = cellSize;
    this->m_impl->offsetX = offsetX;
    this->m_impl->offsetY = offsetY;
    this->m_impl->maxCols = maxCols;
    this->m_impl->maxRows = maxRows;
    
    // Update the pixels per meter ratio for physics calculations
    this->m_impl->pixelsPerMeter = cellSize;
    
    // Draw maze background
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // Light background
    SDL_RenderClear(renderer);
    
    // Draw maze path cells with different colors for the grid
    for (size_t i = 0; i < mazeLen; i++) {
        char c = mazeData[i];
        if (c == '\n') {
            currentCol = 0;
            currentRow++;
            continue;
        }
        
        // Calculate base cell position
        float baseX = offsetX + (currentCol * cellSize);
        float baseY = offsetY + (currentRow * cellSize);
        
        // Apply camera transformation
        SDL_FPoint screenPos = camera->worldToScreen(baseX, baseY, display_w, display_h);
        float x = screenPos.x;
        float y = screenPos.y;
        
        // Scale the cell size according to zoom
        float scaledCellSize = cellSize * camera->zoom;
        
        if (c == ' ') { 
            // Path cell - light gray
            if (currentRow % 2 == 1 && currentCol % 2 == 1) {
                // Make path cells stand out more
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255); // Lighter gray
                
                // We need to draw a rotated rectangle when the camera is rotated
                if (camera->rotation != 0.0f) {
                    // Calculate the four corners of the rotated rectangle
                    float halfSize = scaledCellSize * 0.5f;
                    float cos_r = cosf(camera->rotation);
                    float sin_r = sinf(camera->rotation);
                    
                    SDL_Vertex vertices[6];
                    // Define the four corner points of the rotated rectangle
                    SDL_FPoint points[4] = {
                        {x - halfSize, y - halfSize},  // Top left
                        {x + halfSize, y - halfSize},  // Top right
                        {x + halfSize, y + halfSize},  // Bottom right
                        {x - halfSize, y + halfSize}   // Bottom left
                    };
                    
                    // Create a color for the vertices
                    SDL_FColor color = {220.0f/255.0f, 220.0f/255.0f, 220.0f/255.0f, 1.0f};
                    
                    // Create the vertices for two triangles (forming the rectangle)
                    for (int i = 0; i < 4; i++) {
                        vertices[i].position = points[i];
                        vertices[i].color = color;
                        vertices[i].tex_coord = {0.0f, 0.0f};  // We're not using textures
                    }
                    
                    // Create two triangles from the vertices (0,1,3) and (1,2,3)
                    vertices[4] = vertices[3];  // Duplicate vertex 3
                    vertices[5] = vertices[1];  // Duplicate vertex 1
                    
                    // Render the triangles
                    SDL_RenderGeometry(renderer, nullptr, vertices, 6, nullptr, 0);
                }
                else {
                    // No rotation, just draw a regular rectangle
                    SDL_FRect rect = {x - scaledCellSize * 0.5f, y - scaledCellSize * 0.5f, 
                                    scaledCellSize, scaledCellSize};
                    SDL_RenderFillRect(renderer, &rect);
                    
                    // Add a subtle grid pattern to help with depth perception
                    SDL_SetRenderDrawColor(renderer, 210, 210, 210, 255);
                    SDL_RenderRect(renderer, &rect);
                }
            }
        }
        
        currentCol++;
    }
}

// Generate a new level
void Physics::generateNewLevel(std::string& persistentMazeStr, int display_w, int display_h) const {
    // auto m_ptr = mazes::factory::create_with_rows_columns(INIT_MAZE_ROWS, INIT_MAZE_COLS);
    // if (!m_ptr.has_value()) {
    
    //     SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create maze\n");
    //     return;
    // }

    auto s = "";// mazes::stringz::stringify(cref(m_ptr.value()));

    persistentMazeStr = s;
    
    // Calculate cell size
    int maxCols = 11; // Hardcoded maze width
    int maxRows = 11; // Hardcoded maze height
    float cellW = static_cast<float>(display_w) / static_cast<float>(maxCols);
    float cellH = static_cast<float>(display_h) / static_cast<float>(maxRows);
    float cellSize = std::min(cellW, cellH);
    
    // Create physics objects for the maze
    this->m_impl->createMazePhysics(persistentMazeStr, cellSize);
    
    SDL_Log("New level generated successfully");
}

