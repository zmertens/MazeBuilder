#ifndef MAZE_HPP
#define MAZE_HPP

#include "Drawable.hpp"

class OrthographicCamera;
class SDL_Renderer;

/// @file Maze.hpp
/// @class Maze
/// @brief Data class for a maze with physics properties
class Maze : public Drawable {
public:
  
    // Overrides
    void draw(SDL_Renderer* renderer, 
        std::unique_ptr<OrthographicCamera> const& camera,
        float pixelsPerMeter, float offsetX, float offsetY, float cellSize, int display_w, int display_h) const override;

private:

};

#endif // MAZE_HPP
