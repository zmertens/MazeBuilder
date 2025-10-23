#ifndef MAZE_HPP
#define MAZE_HPP

#include "Drawable.hpp"
#include "Texture.hpp"
#include "Wall.hpp"

#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <vector>


struct OrthographicCamera;
struct SDL_Renderer;

/// @file Maze.hpp
/// @class Maze  
/// @brief Data class for a maze with physics properties
class Maze : public Drawable {
public:
    // Structure to represent a maze cell
    struct Cell {
        char colorValue = '0';  // Base36 color value for rendering
        bool hasTopWall = false;
        bool hasBottomWall = false;
        bool hasLeftWall = false;
        bool hasRightWall = false;
        int row = 0;
        int col = 0;
    };

    // Constructor
    Maze();
    
    // Destructor
    ~Maze();
    
    // Initialize maze from parsed data
    bool initialize(SDL_Renderer* renderer, const std::vector<Cell>& cells, int rows, int cols, float cellSize);
    
    // Overrides
    void draw(SDL_Renderer* renderer, 
        std::unique_ptr<OrthographicCamera> const& camera,
        float pixelsPerMeter, float offsetX, float offsetY, float cellSize, int display_w, int display_h) const override;

    // Getters
    int getRows() const { return rows; }
    int getCols() const { return cols; }
    float getCellSize() const { return cellSize; }
    const std::vector<Wall>& getWalls() const { return walls; }
    const Texture& getTexture() const { return mazeTexture; }

    std::vector<Cell> parse(const std::string& mazeStr, int& rows, int& cols) const noexcept;
    void startBackgroundMazeGeneration() noexcept;
    bool isReady() const noexcept;

private:
    // Maze data
    std::vector<Cell> cells;
    std::vector<Wall> walls;
    Texture mazeTexture;
    int rows = 0;
    int cols = 0;
    float cellSize = 0.0f;

    std::future<std::vector<std::string>> mazeGenerationFuture;

    bool mazeGenerationStarted;
    
    // Helper methods
    void generateWallsFromCells();
    void generateTexture(SDL_Renderer* renderer);
    std::uint8_t base36ToColorComponent(char base36Char, int component) const;
};

#endif // MAZE_HPP
