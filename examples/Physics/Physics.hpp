#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include <memory>
#include <string_view>
#include <string>
#include <vector>

struct SDL_Renderer;
class Ball;
class Wall;

class Physics {
public:
    Physics();
    ~Physics();

    // Complete physics initialization from maze data
    bool initializeFromMaze(const std::string_view& mazeString, float cellSize, int windowWidth, int windowHeight);
    
    // Physics world management
    void initPhysics();
    void createMazePhysics(const std::string_view& mazeString, float cellSize, int windowWidth, int windowHeight);
    void clearPhysicsWorld();
    
    // Game loop physics operations
    void updatePhysics(float deltaTime, bool isPlaying);
    void stepPhysics(float timeStep);
    void processPhysicsCollisions() const;
    void updatePhysicsObjects() const;
    
    // Ball management
    void createBall(float x, float y);
    std::vector<Ball> getBalls() const;
    void updateBallDrag(float mouseX, float mouseY, bool isMouseDown);
    
    // Wall management
    std::vector<Wall> getWalls() const;
    
    // Coordinate conversion
    struct PhysicsPosition { float x; float y; };
    PhysicsPosition screenToPhysics(float screenX, float screenY, int displayWidth, int displayHeight) const;
    struct ScreenPosition { float x; float y; };
    ScreenPosition physicsToScreen(float physX, float physY) const;
    
    // Rendering support
    void drawPhysicsObjects(SDL_Renderer* renderer, float cellSize, float offsetX, float offsetY, int displayWidth, int displayHeight) const;

private:
    struct PhysicsImpl;
    std::unique_ptr<PhysicsImpl> m_impl;
};

#endif // PHYSICS_HPP
