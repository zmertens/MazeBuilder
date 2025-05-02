#include "OrthographicCamera.hpp"

#include <SDL3/SDL.h>

// Camera coordinate transformation implementation
SDL_FPoint OrthographicCamera::worldToScreen(float worldX, float worldY, int screenWidth, int screenHeight) const {
    // Step 1: Apply camera position offset
    // Note: We ADD the camera position to move in the opposite direction
    // This creates the illusion of camera movement when we're actually moving the world
    float offsetX = worldX + this->x;
    float offsetY = worldY + this->y;
    
    // Step 2: Apply zoom factor - centered at screen center
    // Move to origin, scale, then move back
    float screenCenterX = screenWidth / 2.0f;
    float screenCenterY = screenHeight / 2.0f;
    
    float zoomOffsetX = offsetX - screenCenterX;
    float zoomOffsetY = offsetY - screenCenterY;
    
    float zoomedX = screenCenterX + zoomOffsetX * this->zoom;
    float zoomedY = screenCenterY + zoomOffsetY * this->zoom;
    
    // Step 3: Apply rotation around screen center if needed
    float finalX = zoomedX;
    float finalY = zoomedY;
    
    if (this->rotation != 0.0f) {
        float cosR = SDL_cosf(this->rotation);
        float sinR = SDL_sinf(this->rotation);
        
        // Translate to origin, rotate, translate back
        float rotOffsetX = zoomedX - screenCenterX;
        float rotOffsetY = zoomedY - screenCenterY;
        
        finalX = screenCenterX + rotOffsetX * cosR - rotOffsetY * sinR;
        finalY = screenCenterY + rotOffsetX * sinR + rotOffsetY * cosR;
    }
    
    return {finalX, finalY};
}

void OrthographicCamera::screenToWorld(float screenX, float screenY, float& worldX, float& worldY, int screenWidth, int screenHeight) const {
    float screenCenterX = screenWidth / 2.0f;
    float screenCenterY = screenHeight / 2.0f;
    
    // Step 1: Undo rotation
    float unrotatedX = screenX;
    float unrotatedY = screenY;
    
    if (this->rotation != 0.0f) {
        float cosR = SDL_cosf(-this->rotation);  // Use negative rotation to reverse
        float sinR = SDL_sinf(-this->rotation);
        
        float rotOffsetX = screenX - screenCenterX;
        float rotOffsetY = screenY - screenCenterY;
        
        unrotatedX = screenCenterX + rotOffsetX * cosR - rotOffsetY * sinR;
        unrotatedY = screenCenterY + rotOffsetX * sinR + rotOffsetY * cosR;
    }
    
    // Step 2: Undo zoom (centered at screen center)
    float zoomOffsetX = unrotatedX - screenCenterX;
    float zoomOffsetY = unrotatedY - screenCenterY;
    
    float unzoomedX = screenCenterX + zoomOffsetX / this->zoom;
    float unzoomedY = screenCenterY + zoomOffsetY / this->zoom;
    
    // Step 3: Undo camera position offset
    // Note: We SUBTRACT the camera position (opposite of worldToScreen)
    worldX = unzoomedX - this->x;
    worldY = unzoomedY - this->y;
}
