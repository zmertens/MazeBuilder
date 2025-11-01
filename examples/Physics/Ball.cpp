#include "Ball.hpp"

#include <utility>

#include <SDL3/SDL.h>

#include <box2d/box2d.h>

// // Create a dynamic body for the ball
// b2BodyDef bodyDef = b2DefaultBodyDef();
// bodyDef.type = b2_dynamicBody;
// bodyDef.position = { std::get<0>(coords), std::get<1>(coords) };

// bodyDef.linearVelocity = {
//     static_cast<float>(static_cast<int>(std::get<1>(coords)) % 100) - 50.f / 30.0f,
//     static_cast<float>(static_cast<int>(std::get<0>(coords)) % 100) - 50.f / 30.0f
// };

// // Reduced damping for more movement
// bodyDef.linearDamping = 0.2f;
// bodyDef.angularDamping = 0.4f;
// // Enable continuous collision detection
// bodyDef.isBullet = true;
// bodyDef.userData = reinterpret_cast<void*>(this);

// b2BodyId ballBodyId = b2CreateBody(worldId, &bodyDef);

// // Explicitly set the body to be awake
// b2Body_SetAwake(ballBodyId, true);

// // Create a circle shape for the ball
// b2ShapeDef shapeDef = b2DefaultShapeDef();
// // Heavier balls for better collision impacts
// shapeDef.density = 1.5f;
// // Lower resistance for smoother rolling
// shapeDef.material.rollingResistance = 0.1f;
// shapeDef.material.friction = 0.2f;
// // Higher restitution for more bounce
// shapeDef.material.restitution = 0.8f;

// // In Box2D 3.1.0, the circle is defined separately from the shape def
// b2Circle circle = { {0.f, 0.f}, r };

// b2ShapeId ballShapeId = b2CreateCircleShape(ballBodyId, &shapeDef, &circle);

// bodyId = ballBodyId;
// shapeId = ballShapeId;
// isActive = true;

void Ball::update(float elapsed) noexcept {

    // SDL_Log("Ball update() called - implement update logic here");

    // if (isExploding) {
    //     explosionTimer += elapsed;
    //     if (explosionTimer >= 0.5f) {
    //         // Reset ball after explosion
    //         isExploding = false;
    //         explosionTimer = 0.0f;
    //         isActive = false;
    //         // Optionally reset position or other properties here
    //     }
    // }
}

// Draw the ball
void Ball::draw(float elapsed) const noexcept {

    // SDL_Log("Ball draw() called - implement rendering logic here");

    // // Get ball position from Box2D
    // b2Vec2 pos = b2Body_GetPosition(bodyId);
    
    // // Convert physics coordinates to world coordinates
    // float worldX = offsetX + (pos.x * pixelsPerMeter);
    // float worldY = offsetY + (pos.y * pixelsPerMeter);
    
    // // Apply camera transform
    // SDL_FPoint screenPos = camera->worldToScreen(worldX, worldY, display_w, display_h);
    // float screenX = screenPos.x;
    // float screenY = screenPos.y;
    
    // // Scale radius based on zoom
    // float radius = getRadius() * pixelsPerMeter * camera->zoom;
    
    // // Debug log to verify ball positions

    // // SDL_Log("Ball: physics(%.2f,%.2f) world(%.2f,%.2f) screen(%.2f,%.2f) r=%.2f active=%d", 
    //     // pos.x, pos.y, worldX, worldY, screenX, screenY, radius, ball.isActive ? 1 : 0);
    
    // if (getIsExploding()) {
    //     // Render explosion animation
    //     float explosionProgress = getExplosionTimer() / 0.5f;
    //     float expandedRadius = radius * (1.0f + explosionProgress * 2.0f);
        
    //     // Fade out as explosion progresses
    //     int alpha = static_cast<int>(255 * (1.0f - explosionProgress));
        
    //     SDL_SetRenderDrawColor(renderer, 255, 165, 0, alpha); // Orange
        
    //     // Draw explosion as a circle with rays
    //     for (int w = 0; w < 16; w++) {
    //         float angle = static_cast<float>(w) * SDL_PI_F / 8.0f;
    //         SDL_RenderLine(
    //             renderer,
    //             screenX,
    //             screenY,
    //             screenX + SDL_cosf(angle) * expandedRadius,
    //             screenY + SDL_sinf(angle) * expandedRadius
    //         );
    //     }
    // } else {
    //     // Normal ball rendering - make it more visible with solid red circle
    //     SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Bright red
        
    //     // Use efficient circle rendering for large radius
    //     // Number of line segments to approximate the circle
    //     const int segments = 32;
    //     float previousX = screenX + radius;
    //     float previousY = screenY;
        
    //     for (int i = 1; i <= segments; i++) {
    //         float angle = (2.0f * SDL_PI_F * static_cast<float>(i)) / static_cast<float>(segments);
    //         float x = screenX + radius * SDL_cosf(angle);
    //         float y = screenY + radius * SDL_sinf(angle);
            
    //         SDL_RenderLine(renderer, previousX, previousY, x, y);
    //         previousX = x;
    //         previousY = y;
    //     }
        
    //     // Fill the circle efficiently
    //     for (int y = -radius; y <= radius; y += 1) {
    //         float width = SDL_sqrtf(radius * radius - static_cast<float>(y * y));
    //         SDL_RenderLine(renderer, screenX - width, screenY + y, screenX + width, screenY + y);
    //     }
        
    //     // Add highlight effect for better visualization
    //      // Light red highlight
    //     SDL_SetRenderDrawColor(renderer, 255, 200, 200, 255);
    //     float highlight_radius = radius * 0.5f;
    //     // Offset to upper left for light effect
    //     float highlightOffsetX = -radius * 0.2f;
    //     float highlightOffsetY = -radius * 0.2f;
    //     // Draw the highlight effect
    //     for (int y = -highlight_radius; y <= 0; y += 1) {
    //         float width = SDL_sqrtf(highlight_radius * highlight_radius - static_cast<float>(y * y));
    //         SDL_RenderLine(renderer, 
    //             screenX + highlightOffsetX - width / 2.f, 
    //             screenY + highlightOffsetY + y, 
    //             screenX + highlightOffsetX + width / 2.f, 
    //             screenY + highlightOffsetY + y);
    //     }
    // }
}
