#include "Maze.hpp"

#include "OrthographicCamera.hpp"

#include <SDL3/SDL.h>

void Maze::draw(SDL_Renderer* renderer, 
    std::unique_ptr<OrthographicCamera> const& camera,
    float pixelsPerMeter, float offsetX, float offsetY, float cellSize, int display_w, int display_h) const {
    // Currently, Maze has no specific drawing logic.
    // This function can be expanded in the future to render maze-specific elements.
}