#include "Ball.hpp"

#include <utility>

#include <SDL3/SDL.h>

#include "OrthographicCamera.hpp"

Ball::Ball(std::tuple<float, float, float> coords, float r, const b2WorldId worldId)
    : coords{ coords }, radius{ r }
    , bodyId{ b2_nullBodyId }
    , shapeId{ b2_nullShapeId }
    , isActive{ false }
    , isDragging{ false }
    , isExploding{ false }
    , explosionTimer{ 0.f } {

    // Create a dynamic body for the ball
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = { std::get<0>(coords), std::get<1>(coords) };

    bodyDef.linearVelocity = {
        static_cast<float>(static_cast<int>(std::get<1>(coords)) % 100) - 50.f / 30.0f,
        static_cast<float>(static_cast<int>(std::get<0>(coords)) % 100) - 50.f / 30.0f
    };

    // Reduced damping for more movement
    bodyDef.linearDamping = 0.2f;
    bodyDef.angularDamping = 0.4f;
    // Enable continuous collision detection
    bodyDef.isBullet = true;
    bodyDef.userData = reinterpret_cast<void*>(this);

    b2BodyId ballBodyId = b2CreateBody(worldId, &bodyDef);

    // Explicitly set the body to be awake
    b2Body_SetAwake(ballBodyId, true);

    // Create a circle shape for the ball
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    // Heavier balls for better collision impacts
    shapeDef.density = 1.5f;
    // Lower resistance for smoother rolling
    shapeDef.material.rollingResistance = 0.1f;
    shapeDef.material.friction = 0.2f;
    // Higher restitution for more bounce
    shapeDef.material.restitution = 0.8f;

    // In Box2D 3.1.0, the circle is defined separately from the shape def
    b2Circle circle = { {0.f, 0.f}, r };

    b2ShapeId ballShapeId = b2CreateCircleShape(ballBodyId, &shapeDef, &circle);

    bodyId = ballBodyId;
    shapeId = ballShapeId;
    isActive = true;
}

// Getters
b2BodyId Ball::getBodyId() const { return bodyId; }
b2ShapeId Ball::getShapeId() const { return shapeId; }
bool Ball::getIsActive() const { return isActive; }
bool Ball::getIsDragging() const { return isDragging; }
bool Ball::getIsExploding() const { return isExploding; }
float Ball::getExplosionTimer() const { return explosionTimer; }
std::tuple<float, float, float> Ball::getCoords() const { return coords; }
float Ball::getRadius() const { return radius; }

// Setters
void Ball::setBodyId(const b2BodyId& id) { bodyId = id; }
void Ball::setShapeId(const b2ShapeId& id) { shapeId = id; }
void Ball::setIsActive(bool active) { isActive = active; }
void Ball::setIsDragging(bool dragging) { isDragging = dragging; }
void Ball::setIsExploding(bool exploding) { isExploding = exploding; }
void Ball::setExplosionTimer(float timer) { explosionTimer = timer; }
void Ball::setCoords(const std::tuple<float, float, float>& newCoords) { coords = newCoords; }
void Ball::setRadius(float newRadius) { radius = newRadius; }

// Draw the ball
void Ball::draw(SDL_Renderer* renderer, 
    std::unique_ptr<OrthographicCamera> const& camera,
    float pixelsPerMeter, float offsetX, float offsetY, float cellSize, int display_w, int display_h) const {
        
    // Get ball position from Box2D
    b2Vec2 pos = b2Body_GetPosition(getBodyId());
    
    // Convert physics coordinates to world coordinates
    float worldX = offsetX + (pos.x * pixelsPerMeter);
    float worldY = offsetY + (pos.y * pixelsPerMeter);
    
    // Apply camera transform
    SDL_FPoint screenPos = camera->worldToScreen(worldX, worldY, display_w, display_h);
    float screenX = screenPos.x;
    float screenY = screenPos.y;
    
    // Scale radius based on zoom
    float radius = getRadius() * pixelsPerMeter * camera->zoom;
    
    // Debug log to verify ball positions

    // SDL_Log("Ball: physics(%.2f,%.2f) world(%.2f,%.2f) screen(%.2f,%.2f) r=%.2f active=%d", 
        // pos.x, pos.y, worldX, worldY, screenX, screenY, radius, ball.isActive ? 1 : 0);
    
    if (getIsExploding()) {
        // Render explosion animation
        float explosionProgress = getExplosionTimer() / 0.5f;
        float expandedRadius = radius * (1.0f + explosionProgress * 2.0f);
        
        // Fade out as explosion progresses
        int alpha = static_cast<int>(255 * (1.0f - explosionProgress));
        
        SDL_SetRenderDrawColor(renderer, 255, 165, 0, alpha); // Orange
        
        // Draw explosion as a circle with rays
        for (int w = 0; w < 16; w++) {
            float angle = static_cast<float>(w) * SDL_PI_F / 8.0f;
            SDL_RenderLine(
                renderer,
                screenX,
                screenY,
                screenX + SDL_cosf(angle) * expandedRadius,
                screenY + SDL_sinf(angle) * expandedRadius
            );
        }
    } else {
        // Normal ball rendering - make it more visible with solid red circle
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Bright red
        
        // Use efficient circle rendering for large radius
        // Number of line segments to approximate the circle
        const int segments = 32;
        float previousX = screenX + radius;
        float previousY = screenY;
        
        for (int i = 1; i <= segments; i++) {
            float angle = (2.0f * SDL_PI_F * static_cast<float>(i)) / static_cast<float>(segments);
            float x = screenX + radius * SDL_cosf(angle);
            float y = screenY + radius * SDL_sinf(angle);
            
            SDL_RenderLine(renderer, previousX, previousY, x, y);
            previousX = x;
            previousY = y;
        }
        
        // Fill the circle efficiently
        for (int y = -radius; y <= radius; y += 1) {
            float width = SDL_sqrtf(radius * radius - static_cast<float>(y * y));
            SDL_RenderLine(renderer, screenX - width, screenY + y, screenX + width, screenY + y);
        }
        
        // Add highlight effect for better visualization
         // Light red highlight
        SDL_SetRenderDrawColor(renderer, 255, 200, 200, 255);
        float highlight_radius = radius * 0.5f;
        // Offset to upper left for light effect
        float highlightOffsetX = -radius * 0.2f;
        float highlightOffsetY = -radius * 0.2f;
        // Draw the highlight effect
        for (int y = -highlight_radius; y <= 0; y += 1) {
            float width = SDL_sqrtf(highlight_radius * highlight_radius - static_cast<float>(y * y));
            SDL_RenderLine(renderer, 
                screenX + highlightOffsetX - width / 2.f, 
                screenY + highlightOffsetY + y, 
                screenX + highlightOffsetX + width / 2.f, 
                screenY + highlightOffsetY + y);
        }
    }
}
