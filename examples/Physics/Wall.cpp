#include "Wall.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <random>

#include "OrthographicCamera.hpp"

// Constructor
Wall::Wall(b2BodyId bodyId, b2ShapeId shapeId, int hitCount, bool isDestroyed, int row, int col, Orientation orientation)
    : bodyId(bodyId), shapeId(shapeId), hitCount(hitCount), isDestroyed(isDestroyed), row(row), col(col), orientation(orientation) {
}

// Getters
b2BodyId Wall::getBodyId() const { return bodyId; }
b2ShapeId Wall::getShapeId() const { return shapeId; }
int Wall::getHitCount() const { return hitCount; }
bool Wall::getIsDestroyed() const { return isDestroyed; }
int Wall::getRow() const { return row; }
int Wall::getCol() const { return col; }
Wall::Orientation Wall::getOrientation() const { return orientation; }

// Setters
void Wall::setBodyId(const b2BodyId& id) { bodyId = id; }
void Wall::setShapeId(const b2ShapeId& id) { shapeId = id; }
void Wall::setHitCount(int hitCount) { this->hitCount = hitCount; }
void Wall::setIsDestroyed(bool isDestroyed) { this->isDestroyed = isDestroyed; }
void Wall::setRow(int row) { this->row = row; }
void Wall::setCol(int col) { this->col = col; }
void Wall::setOrientation(Orientation orientation) { this->orientation = orientation; }

void Wall::draw(SDL_Renderer* renderer,
    std::unique_ptr<OrthographicCamera> const& camera,
    float pixelsPerMeter, float offsetX, float offsetY, float cellSize, int display_w, int display_h) const
{
    // Calculate world position
    float worldX = offsetX + (getCol() * cellSize);
    float worldY = offsetY + (getRow() * cellSize);
    // Transform with camera
    SDL_FPoint screenPos = camera->worldToScreen(worldX, worldY, display_w, display_h);
    float screenX = screenPos.x;
    float screenY = screenPos.y;
    // Scale dimensions based on zoom
    float halfWidth, halfHeight;
    if (getOrientation() == Orientation::HORIZONTAL) {
        halfWidth = cellSize * 0.5f * camera->zoom;
        halfHeight = cellSize * 0.1f * camera->zoom;
    } else if (getOrientation() == Orientation::VERTICAL) {
        halfWidth = cellSize * 0.1f * camera->zoom;
        halfHeight = cellSize * 0.5f * camera->zoom;
    } else {
        // CORNER
        halfWidth = halfHeight = cellSize * 0.15f * camera->zoom;
    }
    // Calculate color based on hit count
    constexpr float WALL_HIT_THRESHOLD = 4.0f;
    float hitRatio = static_cast<float>(getHitCount()) / WALL_HIT_THRESHOLD;
    uint8_t red = 0, green = 0, blue = 0;
    if (getHitCount() == 0) {
        red = green = blue = 0;
    } else if (hitRatio < 0.33f) {
        red = 220; green = 120; blue = 0;
    } else if (hitRatio < 0.67f) {
        red = 240; green = 80; blue = 0;
    } else {
        red = 255;
        green = static_cast<uint8_t>(hitRatio > 0.9f ? 255 : 40);
        blue = 0;
    }
    SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
    SDL_FRect rect = {
        screenX - halfWidth,
        screenY - halfHeight,
        halfWidth * 2,
        halfHeight * 2
    };
    SDL_RenderFillRect(renderer, &rect);
    // Add damage visual effects
    if (getHitCount() > 0) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
        int numCracks = 1 + static_cast<int>(hitRatio * 6);
        for (int i = 0; i < numCracks; i++) {
            float crackStartX = screenX - halfWidth * (0.8f * (static_cast<float>(rand()) / RAND_MAX));
            float crackStartY = screenY - halfHeight * (0.8f * (static_cast<float>(rand()) / RAND_MAX));
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
            float pulseAlpha = (SDL_sinf(pulseTimer * 10.0f) + 1.0f) * 0.5f * 150.0f;
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, static_cast<uint8_t>(pulseAlpha));
            SDL_RenderRect(renderer, &rect);
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
