#ifndef ORTHOGRAPHIC_CAMERA_HPP
#define ORTHOGRAPHIC_CAMERA_HPP

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

#endif // ORTHOGRAPHIC_CAMERA_HPP
