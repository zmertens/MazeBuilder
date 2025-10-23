//
// Physics class implementation
// Simple rigid body physics simulations using Box2D

#include "Physics.hpp"

#include <box2d/box2d.h>
#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include <cmath>

#include "Ball.hpp"
#include "Wall.hpp"
#include "World.hpp"
#include "OrthographicCamera.hpp"

struct Physics::PhysicsImpl {
    // Game-specific constants
    static constexpr float WALL_HIT_THRESHOLD = 4.0f;
    static constexpr float WALL_WIDTH = 0.1f;
    static constexpr int MAX_BALLS = 10;
    
    // Box2D world and physics components
    std::unique_ptr<World> physicsWorld;
    float timeStep = 1.0f / 60.0f;
    float pixelsPerMeter = 40.0f; // Scale factor for Box2D (which uses meters)
    
    // Game-specific variables
    std::vector<Wall> walls;
    std::vector<Ball> balls;
    
    struct ExitCell {
        int row;
        int col;
        b2BodyId bodyId = b2_nullBodyId;
        b2ShapeId shapeId = b2_nullShapeId;
        int ballsCollected = 0;
    };
    ExitCell exitCell;
    
    // Ball dragging state
    bool isDragging = false;
    int draggedBallIndex = -1;
    b2Vec2 lastMousePos = {0.0f, 0.0f};
    
    // Maze dimensions for coordinate conversion
    float cellSize = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    int maxCols = 0;
    int maxRows = 0;

    PhysicsImpl() = default;
    
    ~PhysicsImpl() = default;
    
    // Initialize the Box2D physics world
    void initPhysics() {
        // Create the world with gravity
        physicsWorld = std::make_unique<World>(9.8f);
        
        // Clear any existing entities
        walls.clear();
        balls.clear();
        
        // Set good values for physics simulation
        timeStep = 1.0f / 60.0f;
        pixelsPerMeter = 40.0f;
    }
    
    // Create a ball at the specified position
    Ball createBall(float x, float y) {
        static constexpr auto COMMON_BALL_RADIUS = 0.45f;
        return Ball{ {x, y, 0.f}, COMMON_BALL_RADIUS, physicsWorld->getWorldId() };
    }
    
    // Convert screen coordinates to physics world coordinates  
    b2Vec2 screenToPhysics(float screenX, float screenY, int displayWidth, int displayHeight) {
        // Simple conversion for now - can be enhanced with camera later
        float physX = (screenX - offsetX) / pixelsPerMeter;
        float physY = (screenY - offsetY) / pixelsPerMeter;
        return {physX, physY};
    }
    
    // Convert physics world coordinates to screen coordinates
    SDL_FPoint physicsToScreen(float physX, float physY) {
        return {physX * pixelsPerMeter + offsetX, physY * pixelsPerMeter + offsetY};
    }
    
    // Utility method for handling wall collisions
    void handleWallCollision(b2BodyId possibleWallId, b2BodyId possibleBallId) {
        void* wallUserData = b2Body_GetUserData(possibleWallId);
        uintptr_t wallValue = reinterpret_cast<uintptr_t>(wallUserData);
        
        // Make sure this is a ball hitting a wall (wall userData is set to wall index + 1000)
        if (wallValue >= 1000 && wallValue < 2000) {
            int wallIndex = static_cast<int>(wallValue - 1000);
            
            if (wallIndex >= 0 && wallIndex < static_cast<int>(walls.size())) {
                Wall& wall = walls[wallIndex];
                
                // Increment hit count
                wall.setHitCount(wall.getHitCount() + 1);
                
                SDL_Log("Wall at (%d,%d) hit! Count: %d/%d", 
                       wall.getRow(), wall.getCol(), wall.getHitCount(), static_cast<int>(WALL_HIT_THRESHOLD));
                
                // Mark wall for destruction if threshold is reached
                if (wall.getHitCount() >= WALL_HIT_THRESHOLD) {
                    wall.setIsDestroyed(true);
                    SDL_Log("Wall marked for destruction!");
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
            
            // Find the ball objects
            Ball* ballA = nullptr;
            Ball* ballB = nullptr;
            
            for (auto& ball : balls) {
                if (ball.getBodyId().index1 == bodyAId.index1) {
                    ballA = &ball;
                } else if (ball.getBodyId().index1 == bodyBId.index1) {
                    ballB = &ball;
                }
            }
            
            if (ballA && ballB) {
                SDL_Log("Ball collision detected!");
                
                // Add some visual effect by setting explosion state
                ballA->setIsExploding(true);
                ballA->setExplosionTimer(1.0f);
                ballB->setIsExploding(true);
                ballB->setExplosionTimer(1.0f);
            }
        }
    }
};

// Physics class implementation
Physics::Physics() : m_impl(std::make_unique<PhysicsImpl>()) {
}

Physics::~Physics() = default;

bool Physics::initializeFromMaze(const std::string_view& mazeString, float cellSize, int windowWidth, int windowHeight) {
    // Complete physics initialization in one call
    initPhysics();
    createMazePhysics(mazeString, cellSize, windowWidth, windowHeight);
    
    // Perform initial physics step to ensure bodies are positioned
    if (m_impl->physicsWorld) {
        SDL_Log("Performing initial physics step");
        m_impl->physicsWorld->step(m_impl->timeStep, 4);
        return true;
    }
    return false;
}

void Physics::updatePhysics(float deltaTime, bool isPlaying) {
    if (!isPlaying || !m_impl->physicsWorld) {
        return;
    }
    
    // Step Box2D world with sub-steps as recommended
    m_impl->physicsWorld->step(deltaTime, 4);
    
    // Handle collisions and physics interactions
    processPhysicsCollisions();
    updatePhysicsObjects();
}

void Physics::initPhysics() {
    m_impl->initPhysics();
}

void Physics::createMazePhysics(const std::string_view& mazeString, float cellSize, int windowWidth, int windowHeight) {
    // Clear any existing physics objects
    if (m_impl->physicsWorld) {
        m_impl->physicsWorld->destroyWorld();
    }
    
    // Create a new physics world
    initPhysics();
    m_impl->walls.clear();
    m_impl->balls.clear();
    
    // Store maze parameters
    m_impl->cellSize = cellSize;
    m_impl->pixelsPerMeter = cellSize;
    
    // Calculate maze dimensions
    int maxCols = 0;
    int maxRows = 0;
    
    const char* mazeData = mazeString.data();
    size_t mazeLen = mazeString.size();
    int currentRow = 0;
    int colCount = 0;
    
    for (size_t i = 0; i < mazeLen; i++) {
        char c = mazeData[i];
        if (c == '\n') {
            maxCols = std::max(maxCols, colCount);
            colCount = 0;
            currentRow++;
        } else {
            colCount++;
        }
    }
    maxRows = currentRow + 1;
    
    m_impl->maxCols = maxCols;
    m_impl->maxRows = maxRows;
    
    // Center the maze in the display
    float mazeWidth = maxCols * cellSize;
    float mazeHeight = maxRows * cellSize;
    m_impl->offsetX = (windowWidth - mazeWidth) / 2.0f;
    m_impl->offsetY = (windowHeight - mazeHeight) / 2.0f;
    
    // Ensure offsets are never negative
    m_impl->offsetX = std::max(0.0f, m_impl->offsetX);
    m_impl->offsetY = std::max(0.0f, m_impl->offsetY);
    
    SDL_Log("Physics maze dimensions: %d rows x %d columns", maxRows, maxCols);
}

void Physics::clearPhysicsWorld() {
    if (m_impl->physicsWorld) {
        m_impl->physicsWorld->destroyWorld();
    }
    m_impl->walls.clear();
    m_impl->balls.clear();
}

void Physics::stepPhysics(float timeStep) {
    if (m_impl->physicsWorld) {
        m_impl->physicsWorld->step(timeStep, 4);
    }
}

void Physics::processPhysicsCollisions() const {
    if (!m_impl->physicsWorld) return;
    
    // Process contact events in Box2D 3.1.0 style
    b2ContactEvents contactEvents = m_impl->physicsWorld->getContactEvents();
    
    // Handle contact hit events
    for (int i = 0; i < contactEvents.hitCount; ++i) {
        b2ContactHitEvent* hitEvent = &contactEvents.hitEvents[i];
        b2BodyId bodyA = b2Shape_GetBody(hitEvent->shapeIdA);
        b2BodyId bodyB = b2Shape_GetBody(hitEvent->shapeIdB);
        
        // Process wall collisions
        m_impl->handleWallCollision(bodyA, bodyB);
        m_impl->handleWallCollision(bodyB, bodyA);
        
        // Process ball-to-ball collisions
        m_impl->handleBallCollision(bodyA, bodyB);
    }
}

void Physics::updatePhysicsObjects() const {
    // Handle any destroyed walls 
    for (int i = m_impl->walls.size() - 1; i >= 0; --i) {
        if (m_impl->walls[i].getIsDestroyed()) {
            // Destroy the physics body
            if (m_impl->physicsWorld && B2_IS_NON_NULL(m_impl->walls[i].getBodyId())) {
                m_impl->physicsWorld->destroyBody(m_impl->walls[i].getBodyId());
            }
            
            // Remove from vector
            m_impl->walls.erase(m_impl->walls.begin() + i);
            SDL_Log("Wall destroyed! Remaining walls: %zu", m_impl->walls.size());
        }
    }
    
    // Handle exploding balls
    for (int i = m_impl->balls.size() - 1; i >= 0; --i) {
        if (m_impl->balls[i].getIsExploding()) {
            // Destroy the physics body
            if (m_impl->physicsWorld && B2_IS_NON_NULL(m_impl->balls[i].getBodyId())) {
                m_impl->physicsWorld->destroyBody(m_impl->balls[i].getBodyId());
            }
            
            // Remove from vector
            m_impl->balls.erase(m_impl->balls.begin() + i);
            SDL_Log("Ball exploded! Remaining balls: %zu", m_impl->balls.size());
        }
    }
}

void Physics::createBall(float x, float y) {
    Ball newBall = m_impl->createBall(x, y);
    m_impl->balls.push_back(newBall);
}

std::vector<Ball> Physics::getBalls() const {
    return m_impl->balls;
}

std::vector<Wall> Physics::getWalls() const {
    return m_impl->walls;
}

void Physics::updateBallDrag(float mouseX, float mouseY, bool isMouseDown) {
    // Ball dragging implementation - simplified version
    // This can be enhanced with the full implementation from PhysicsGame
    if (isMouseDown && !m_impl->isDragging) {
        // Find closest ball to mouse position
        // Implementation details...
        m_impl->isDragging = true;
    } else if (!isMouseDown && m_impl->isDragging) {
        m_impl->isDragging = false;
        m_impl->draggedBallIndex = -1;
    }
}

Physics::PhysicsPosition Physics::screenToPhysics(float screenX, float screenY, int displayWidth, int displayHeight) const {
    b2Vec2 pos = m_impl->screenToPhysics(screenX, screenY, displayWidth, displayHeight);
    return {pos.x, pos.y};
}

Physics::ScreenPosition Physics::physicsToScreen(float physX, float physY) const {
    SDL_FPoint pos = m_impl->physicsToScreen(physX, physY);
    return {pos.x, pos.y};
}

void Physics::drawPhysicsObjects(SDL_Renderer* renderer, float cellSize, float offsetX, float offsetY, int displayWidth, int displayHeight) const {
    // Basic physics object rendering - can be enhanced
    // This is a simplified version of the drawing logic
    
    // Count active balls for debugging
    int activeBallCount = 0;
    for (const auto& ball : m_impl->balls) {
        if (!ball.getIsExploding()) {
            activeBallCount++;
        }
    }
    
    SDL_Log("Drawing %d active balls and %zu walls", activeBallCount, m_impl->walls.size());
}

