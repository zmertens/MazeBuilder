//
// PhysicsGame class implementation
// Simple 2D physics simulation with bouncy balls that break walls
//
// Audio Handling reference from SDL_AUDIO_STREAM: SDL\test\testaudio.c
//
// Score system: 10 points per wall destroyed
//              -1 point per friendly ball bounce
//              100 points per exit bounce
//

#include "PhysicsGame.hpp"

#include "CoutThreadSafe.hpp"

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

#include <stb/stb_image.h>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_opengl3.h>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/json_helper.h>

#include "AudioHelper.hpp"
#include "Ball.hpp"
#include "Maze.hpp"
#include "OrthographicCamera.hpp"
#include "Physics.hpp"
#include "Renderer.hpp"
#include "ResourceManager.hpp"
#include "SDLHelper.hpp"
#include "State.hpp"
#include "Texture.hpp"
#include "Wall.hpp"
#include "WorkerConcurrent.hpp"
#include "World.hpp"

static const std::string RESOURCE_PATH_PREFIX = "resources";
static const std::string PHYSICS_JSON_PATH = RESOURCE_PATH_PREFIX + "/" + "physics.json";

struct PhysicsGame::PhysicsGameImpl {
    
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
    
    // Splash screen texture
    SDL_Texture* splashTexture = nullptr;
    int splashWidth = 0, splashHeight = 0;
    
    // Maze distance data
    // cell index -> base36 distance character
    std::unordered_map<int, char> distanceMap;
    SDL_Texture* mazeDistanceTexture = nullptr;
    int mazeWidth = 0, mazeHeight = 0;
    
    // Background maze generation
    std::vector<std::string> generatedMazes;
    
    // Current active maze
    std::unique_ptr<Maze> currentMaze;

    // Camera for coordinate transformations
    std::unique_ptr<OrthographicCamera> camera;

    // Components using PIMPL idiom
    std::unique_ptr<Physics> physics;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<Renderer> mazeRenderer;

    PhysicsGameImpl(const std::string& title, const std::string& version, int w, int h)
        : title{ title }, version{ version }, INIT_WINDOW_W{ w }, INIT_WINDOW_H{ h }
        , sdlHelper{}, state{ State::SPLASH }, workerConcurrent{state}
        , camera{ std::make_unique<OrthographicCamera>() }
        , physics{ std::make_unique<Physics>() }
        , resourceManager{ std::make_unique<ResourceManager>() }
        , mazeRenderer{ std::make_unique<Renderer>() } {

    }

    ~PhysicsGameImpl() {
        // Clean up splash texture
        if (splashTexture) {
            SDL_DestroyTexture(splashTexture);
            splashTexture = nullptr;
        }
        
        // Clean up maze distance texture
        if (mazeDistanceTexture) {
            SDL_DestroyTexture(mazeDistanceTexture);
            mazeDistanceTexture = nullptr;
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
        
        SDL_Log("PhysicsGame world size: %.2f x %.2f meters", worldWidth, worldHeight);
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

    // Create SDL texture from distance map data
    void createDistanceTexture(int display_w, int display_h) {
        if (this->distanceMap.empty()) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No distance data to create texture from");
            return;
        }
        
        // Clean up existing texture
        if (this->mazeDistanceTexture) {
            SDL_DestroyTexture(this->mazeDistanceTexture);
            this->mazeDistanceTexture = nullptr;
        }
        
        // Calculate texture dimensions based on maze grid
        int maxCellIndex = 0;
        for (const auto& [index, distance] : this->distanceMap) {
            maxCellIndex = std::max(maxCellIndex, index);
        }
        
        // Assume square maze for now, improve this later
        int mazeDimension = static_cast<int>(std::sqrt(maxCellIndex + 1));
        this->mazeWidth = mazeDimension * 50;  // 50 pixels per cell
        this->mazeHeight = mazeDimension * 50;
        
        // Create texture
        this->mazeDistanceTexture = SDL_CreateTexture(
            sdlHelper.renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            this->mazeWidth,
            this->mazeHeight
        );
        
        if (!this->mazeDistanceTexture) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create distance texture: %s", SDL_GetError());
            return;
        }
        
        // Set texture as render target and draw distance visualization
        SDL_SetRenderTarget(sdlHelper.renderer, this->mazeDistanceTexture);
        SDL_SetRenderDrawColor(sdlHelper.renderer, 255, 255, 255, 255); // White background
        SDL_RenderClear(sdlHelper.renderer);
        
        // Draw each cell with color based on distance
        int cellSize = 50;
        for (const auto& [index, distanceChar] : this->distanceMap) {
            int row = index / mazeDimension;
            int col = index % mazeDimension;
            
            // Convert base36 character to numeric value for color blending
            int distanceValue = 0;
            if (distanceChar >= '0' && distanceChar <= '9') {
                distanceValue = distanceChar - '0';
            } else if (distanceChar >= 'A' && distanceChar <= 'Z') {
                distanceValue = distanceChar - 'A' + 10;
            } else if (distanceChar >= 'a' && distanceChar <= 'z') {
                distanceValue = distanceChar - 'a' + 10;
            }
            
            // Create color based on distance (closer = red, farther = blue)
            Uint8 red = static_cast<Uint8>(255 - (distanceValue * 7) % 256);
            Uint8 green = static_cast<Uint8>((distanceValue * 5) % 256);
            Uint8 blue = static_cast<Uint8>((distanceValue * 9) % 256);
            
            SDL_SetRenderDrawColor(sdlHelper.renderer, red, green, blue, 255);
            
            SDL_FRect cellRect = {
                static_cast<float>(col * cellSize),
                static_cast<float>(row * cellSize),
                static_cast<float>(cellSize - 1),  // Leave 1 pixel border
                static_cast<float>(cellSize - 1)
            };
            
            SDL_RenderFillRect(sdlHelper.renderer, &cellRect);
        }
        
        // Reset render target
        SDL_SetRenderTarget(sdlHelper.renderer, nullptr);
        
        SDL_Log("Created distance texture: %dx%d", this->mazeWidth, this->mazeHeight);
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

PhysicsGame::PhysicsGame(const std::string& title, const std::string& version, int w, int h)
    : m_impl{ std::make_unique<PhysicsGameImpl>(std::cref(title), std::cref(version), w, h)} {
}

PhysicsGame::~PhysicsGame() = default;

bool PhysicsGame::run() const noexcept {
    using namespace std;

    auto&& gamePtr = this->m_impl;

    auto&& sdlHelper = gamePtr->sdlHelper;
    auto&& workers = gamePtr->workerConcurrent;

    sdlHelper.init();

    string_view titleView = gamePtr->title;
    sdlHelper.window = SDL_CreateWindow(titleView.data(), gamePtr->INIT_WINDOW_W, gamePtr->INIT_WINDOW_H, SDL_WINDOW_RESIZABLE);
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

    // Load physics.json configuration using ResourceManager component
    if (!this->m_impl->resourceManager->loadConfiguration(PHYSICS_JSON_PATH)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load physics.json from: %s\n", PHYSICS_JSON_PATH.c_str());
        return false;
    }

    // Load audio resources using configuration
    const auto musicOggConfig = this->m_impl->resourceManager->getConfigValue("music_ogg");
    const auto generateOggFile = this->m_impl->resourceManager->extractJsonValue(musicOggConfig);
    sf::SoundBuffer generateSoundBuffer;
    const auto generatePath = this->m_impl->resourceManager->getResourcePath(generateOggFile);
    if (!generateSoundBuffer.loadFromFile(generatePath)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load sound file: %s\n", generatePath.c_str());
        return false;
    } else {
        SDL_Log("Success loading sound file: %s\n", generatePath.c_str());
    }

    unique_ptr<sf::Sound> generateSound = make_unique<sf::Sound>(generateSoundBuffer);
    AudioHelper audioHelper{cref(generateSound)};

    // Load and set window icon from configuration
    const auto iconImageConfig = this->m_impl->resourceManager->getConfigValue("icon_image");
    const auto iconImageFile = this->m_impl->resourceManager->extractJsonValue(iconImageConfig);
    const auto iconPath = this->m_impl->resourceManager->getResourcePath(iconImageFile);
    SDL_Surface* icon = SDL_LoadBMP(iconPath.c_str());
    if (icon) {
        SDL_SetWindowIcon(sdlHelper.window, icon);
        SDL_DestroySurface(icon);
        SDL_Log("Successfully loaded icon: %s\n", iconPath.c_str());
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load icon: %s - %s\n", iconPath.c_str(), SDL_GetError());
    }

    // Load WAV file from configuration
    const auto musicWavConfig = this->m_impl->resourceManager->getConfigValue("music_wav");
    const auto loadingWavFile = this->m_impl->resourceManager->extractJsonValue(musicWavConfig);
    const auto loadingPath = this->m_impl->resourceManager->getResourcePath(loadingWavFile);
    if (!sdlHelper.loadWAV(loadingPath.c_str())) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load WAV file: %s\n", loadingPath.c_str());
    } else {
        SDL_Log("Successfully loaded WAV file: %s\n", loadingPath.c_str());
    }

    sdlHelper.playAudioStream();
    
    // Load splash image from configuration
    const auto splashImageConfig = this->m_impl->resourceManager->getConfigValue("splash_image");
    const auto splashImageFile = this->m_impl->resourceManager->extractJsonValue(splashImageConfig);
    const auto splashImagePath = this->m_impl->resourceManager->getResourcePath(splashImageFile);
    this->m_impl->splashTexture = this->m_impl->resourceManager->loadTexture(sdlHelper.renderer, splashImagePath);
    if (this->m_impl->splashTexture) {
        // Get texture dimensions
        float width, height;
        SDL_GetTextureSize(this->m_impl->splashTexture, &width, &height);
        this->m_impl->splashWidth = static_cast<int>(width);
        this->m_impl->splashHeight = static_cast<int>(height);
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
    
    // Set a good default value for pixelsPerMeter
    this->m_impl->pixelsPerMeter = 20.0f;
    
    // Start background maze generation while resources are loading
    Maze myMaze{};
    myMaze.startBackgroundMazeGeneration();

    // Generate initial level at startup
    int display_w, display_h;
    SDL_GetWindowSize(window, &display_w, &display_h);
    this->generateNewLevel(ref(persistentMazeStr), display_w, display_h);
    
    // Log physics world state after level generation
    SDL_Log("PhysicsGame world created, num walls: %zu, num balls: %zu", 
        this->m_impl->walls.size(), this->m_impl->balls.size());
    
    // Initial physics simulation step to ensure bodies are positioned
    if (this->m_impl->physicsWorld) {
        SDL_Log("Performing initial physics step");
        this->m_impl->physicsWorld->step(this->m_impl->timeStep, 4);
    }

    double previous = static_cast<double>(SDL_GetTicks());
    double accumulator = 0.0, currentTimeStep = 0.0;
    gamePtr->state = State::SPLASH;
    while (gamePtr->state != State::DONE) {

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

        // Update physics simulation if we're in a playing state
        if ((gState == State::PLAY_SINGLE_MODE || gState == State::PLAY_MULTI_MODE) && this->m_impl->physicsWorld) {
             
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
        
        // Render maze distance visualization in MAIN_MENU state
        if (gState == State::MAIN_MENU && this->m_impl->mazeDistanceTexture) {
            // Center the maze distance texture on screen
            int centerX = (display_w - this->m_impl->mazeWidth) / 2;
            int centerY = (display_h - this->m_impl->mazeHeight) / 2;
            
            SDL_FRect mazeRect = {
                static_cast<float>(centerX),
                static_cast<float>(centerY),
                static_cast<float>(this->m_impl->mazeWidth),
                static_cast<float>(this->m_impl->mazeHeight)
            };
            
            SDL_RenderTexture(renderer, this->m_impl->mazeDistanceTexture, nullptr, &mazeRect);
            
            // Add instruction text (could be enhanced with proper text rendering)
            // For now, we'll just show the distance visualization
        }
        
        // Generate new level in MAIN_MENU state (only once when entering the state)
        static State previousState = State::SPLASH;
        if (gState == State::MAIN_MENU && previousState != State::MAIN_MENU) {
            // Create and display maze with distances when first entering MAIN_MENU

            
            // Check if background maze generation is complete
            if (myMaze.isReady() && !this->m_impl->generatedMazes.empty()) {
                // Use the first generated maze
                int rows, cols;
                auto mazeCells = myMaze.parse(this->m_impl->generatedMazes[0], rows, cols);

                if (!mazeCells.empty()) {
                    // Initialize current maze with parsed data
                    this->m_impl->currentMaze = std::make_unique<Maze>();
                    float cellSize = std::min(display_w / (float)cols, display_h / (float)rows) * 0.8f; // 80% of available space
                    this->m_impl->currentMaze->initialize(renderer, mazeCells, rows, cols, cellSize);
                    
                    SDL_Log("Initialized maze rendering with %dx%d maze", rows, cols);
                }
            }
        }
        previousState = gState;
        
        // Start playing when transitioning from MAIN_MENU to PLAY_SINGLE_MODE
        if (gState == State::PLAY_SINGLE_MODE && persistentMazeStr.empty()) {
            // Generate initial level if we don't have one
            this->generateNewLevel(ref(persistentMazeStr), display_w, display_h);
        }
        
        // Draw the current maze if available (in PLAY_SINGLE_MODE or MAIN_MENU)
        if ((gState == State::PLAY_SINGLE_MODE || gState == State::MAIN_MENU) && this->m_impl->currentMaze) {
            // Calculate centering offset
            float offsetX = (display_w - this->m_impl->currentMaze->getCols() * this->m_impl->currentMaze->getCellSize()) / 2.0f;
            float offsetY = (display_h - this->m_impl->currentMaze->getRows() * this->m_impl->currentMaze->getCellSize()) / 2.0f;
            
            this->m_impl->currentMaze->draw(renderer, this->m_impl->camera, 
                                           this->m_impl->pixelsPerMeter, offsetX, offsetY, 
                                           this->m_impl->currentMaze->getCellSize(), display_w, display_h);
        } else {
            // Fall back to old maze rendering if new system isn't ready
            this->drawMaze(renderer, persistentMazeStr, display_w, display_h);
        }
        
        // Draw the physics entities (balls, walls, exit) if we're in a playing state
        if ((gState == State::PLAY_SINGLE_MODE || gState == State::PLAY_MULTI_MODE) && 
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
void PhysicsGame::processPhysicsCollisions() const {
    // Delegate to Physics component
    this->m_impl->physics->processPhysicsCollisions();
}

// Update physics objects state (balls, walls)
void PhysicsGame::updatePhysicsObjects() const {
    // Delegate to Physics component
    this->m_impl->physics->updatePhysicsObjects();
}

// Draw the physics objects (balls, walls, exit)
void PhysicsGame::drawPhysicsObjects(SDL_Renderer* renderer) const {
    // Get values from impl
    float cellSize = this->m_impl->cellSize;
    float offsetX = this->m_impl->offsetX;
    float offsetY = this->m_impl->offsetY;
    int display_w, display_h;
    SDL_GetCurrentRenderOutputSize(renderer, &display_w, &display_h);
    
    // Delegate to Physics component
    this->m_impl->physics->drawPhysicsObjects(renderer, cellSize, offsetX, offsetY, display_w, display_h);
}

// Draw the maze background
void PhysicsGame::drawMaze(SDL_Renderer* renderer, const std::string_view& cells, int display_w, int display_h) const {
    // Delegate to Renderer component
    const auto& camera = this->m_impl->camera;
    this->m_impl->mazeRenderer->drawMazeWithCamera(renderer, cells, display_w, display_h, 
                                                   camera->x, camera->y, camera->zoom, camera->rotation);
    
    // Update physics game state with calculated values from Renderer
    // The Renderer should calculate and store these values internally
    // For now, we'll keep the essential calculations here
    if (!cells.empty()) {
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
        
        // Calculate cell size
        float cellW = static_cast<float>(display_w) / static_cast<float>(maxCols);
        float cellH = static_cast<float>(display_h) / static_cast<float>(maxRows);
        float cellSize = std::min(cellW, cellH) * 0.95f;
        
        // Make sure path cells are large enough
        float minCellSize = 20.0f;
        cellSize = std::max(cellSize, minCellSize);
        
        // Center the maze
        float mazeWidth = maxCols * cellSize;
        float mazeHeight = maxRows * cellSize;
        float offsetX = std::max(0.0f, (display_w - mazeWidth) / 2.0f);
        float offsetY = std::max(0.0f, (display_h - mazeHeight) / 2.0f);
        
        // Store values for physics objects
        this->m_impl->cellSize = cellSize;
        this->m_impl->offsetX = offsetX;
        this->m_impl->offsetY = offsetY;
        this->m_impl->maxCols = maxCols;
        this->m_impl->maxRows = maxRows;
        this->m_impl->pixelsPerMeter = cellSize;
    }
}

// Generate a new level
void PhysicsGame::generateNewLevel(std::string& persistentMazeStr, int display_w, int display_h) const {
    // Use Renderer component to generate maze
    persistentMazeStr = this->m_impl->mazeRenderer->generateNewLevel(mazes::configurator::DEFAULT_ROWS, 
        mazes::configurator::DEFAULT_COLUMNS, display_w, display_h);
    
    // Calculate cell size
    float cellW = static_cast<float>(display_w) / static_cast<float>(mazes::configurator::DEFAULT_COLUMNS);
    float cellH = static_cast<float>(display_h) / static_cast<float>(mazes::configurator::DEFAULT_ROWS);
    float cellSize = std::min(cellW, cellH);
    
    // Use Physics component to create physics objects for the maze
    this->m_impl->physics->createMazePhysics(persistentMazeStr, cellSize, display_w, display_h);
    
    SDL_Log("New level generated successfully using components");
}
