#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

#include <MazeBuilder/singleton_base.h>

struct SDL_Renderer;
struct SDL_FPoint;

// Forward declaration for OrthographicCamera
struct OrthographicCamera {
    float x = 0.0f;           // Camera position X
    float y = 0.0f;           // Camera position Y
    float zoom = 1.0f;        // Camera zoom level
    float rotation = 0.0f;    // Camera rotation in radians
    
    // Camera movement speed and zoom factor controls
    float panSpeed = 0.04f;    
    float zoomSpeed = 1.1f;
    float rotationSpeed = 2.02f;
    
    // Transform a point from world to screen coordinates
    SDL_FPoint worldToScreen(float worldX, float worldY, int screenWidth, int screenHeight) const;
    
    // Transform a point from screen to world coordinates
    void screenToWorld(float screenX, float screenY, float& worldX, float& worldY, int screenWidth, int screenHeight) const;
};

class Physics : public mazes::singleton_base<Physics> {
    friend class mazes::singleton_base<Physics>;
public:
    Physics(const std::string& title, const std::string& version, int w, int h);
    ~Physics();

    bool run() const noexcept;

private:
    // Physics and collision processing
    void processPhysicsCollisions() const;
    void updatePhysicsObjects() const;
    
    // Camera control methods
    void updateCamera(float deltaTime) const;
    void handleCameraInput() const;
    
    // Rendering methods
    void drawPhysicsObjects(SDL_Renderer* renderer) const;
    void drawMaze(SDL_Renderer* renderer, const std::string_view& cells, int display_w, int display_h) const;
    void generateNewLevel(std::string& persistentMazeStr, int display_w, int display_h) const;
    void drawDebugTestObjects(SDL_Renderer* renderer) const;

    struct PhysicsImpl;
    std::unique_ptr<PhysicsImpl> m_impl;
};

#endif // PHYSICS_HPP
