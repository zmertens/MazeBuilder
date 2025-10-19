#include "Maze.hpp"
#include "OrthographicCamera.hpp"
#include <SDL3/SDL.h>
#include <cmath>
#include <algorithm>

// Constructor
Maze::Maze() {
}

// Destructor  
Maze::~Maze() {
}

// Initialize maze from parsed data
bool Maze::initialize(SDL_Renderer* renderer, const std::vector<MazeCell>& mazeCells, int mazeRows, int mazeCols, float mazeCellSize) {
    if (mazeCells.empty() || mazeRows <= 0 || mazeCols <= 0 || !renderer) {
        return false;
    }
    
    cells = mazeCells;
    rows = mazeRows;
    cols = mazeCols;
    cellSize = mazeCellSize;
    
    // Generate wall objects from cell data
    generateWallsFromCells();
    
    // Generate texture for efficient rendering
    generateTexture(renderer);
    
    SDL_Log("Maze initialized: %dx%d, %zu cells, %zu walls", rows, cols, cells.size(), walls.size());
    return true;
}

void Maze::draw(SDL_Renderer* renderer, 
    std::unique_ptr<OrthographicCamera> const& camera,
    float pixelsPerMeter, float offsetX, float offsetY, float cellSize, int display_w, int display_h) const {
    
    if (!mazeTexture.get()) {
        return; // No texture to render
    }
    
    // Calculate world position for the maze
    float worldX = offsetX;
    float worldY = offsetY;
    
    // Transform with camera
    SDL_FPoint screenPos = camera->worldToScreen(worldX, worldY, display_w, display_h);
    
    // Scale dimensions based on zoom
    float scaledWidth = cols * cellSize * camera->zoom;
    float scaledHeight = rows * cellSize * camera->zoom;
    
    // Render the maze texture
    SDL_FRect renderRect = {
        screenPos.x,
        screenPos.y,
        scaledWidth,
        scaledHeight
    };
    
    SDL_RenderTexture(renderer, mazeTexture.get(), nullptr, &renderRect);
}

// Generate Wall objects from cell data
void Maze::generateWallsFromCells() {
    walls.clear();
    
    for (const auto& cell : cells) {
        // Create walls for each side that has a wall
        if (cell.hasTopWall) {
            Wall wall(b2_nullBodyId, b2_nullShapeId, 0, false, cell.row, cell.col, Wall::Orientation::HORIZONTAL);
            walls.push_back(wall);
        }
        
        if (cell.hasBottomWall) {
            Wall wall(b2_nullBodyId, b2_nullShapeId, 0, false, cell.row + 1, cell.col, Wall::Orientation::HORIZONTAL);
            walls.push_back(wall);
        }
        
        if (cell.hasLeftWall) {
            Wall wall(b2_nullBodyId, b2_nullShapeId, 0, false, cell.row, cell.col, Wall::Orientation::VERTICAL);
            walls.push_back(wall);
        }
        
        if (cell.hasRightWall) {
            Wall wall(b2_nullBodyId, b2_nullShapeId, 0, false, cell.row, cell.col + 1, Wall::Orientation::VERTICAL);
            walls.push_back(wall);
        }
    }
}

// Generate texture for efficient rendering
void Maze::generateTexture(SDL_Renderer* renderer) {
    if (cells.empty() || rows <= 0 || cols <= 0) {
        return;
    }
    
    int textureWidth = static_cast<int>(cols * cellSize);
    int textureHeight = static_cast<int>(rows * cellSize);
    
    // Create target texture
    if (!mazeTexture.loadTarget(renderer, textureWidth, textureHeight)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create maze texture");
        return;
    }
    
    // Set texture as render target
    SDL_SetRenderTarget(renderer, mazeTexture.get());
    
    // Clear with transparent background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    
    // Draw each cell with its base36 color
    for (const auto& cell : cells) {
        // Calculate cell rectangle
        SDL_FRect cellRect = {
            cell.col * cellSize,
            cell.row * cellSize,
            cellSize,
            cellSize
        };
        
        // Convert base36 color to RGB
        Uint8 red = base36ToColorComponent(cell.colorValue, 0);
        Uint8 green = base36ToColorComponent(cell.colorValue, 1);
        Uint8 blue = base36ToColorComponent(cell.colorValue, 2);
        
        // Draw cell background
        SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
        SDL_RenderFillRect(renderer, &cellRect);
        
        // Draw walls with black color
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        
        float wallThickness = cellSize * 0.05f; // 5% of cell size
        
        if (cell.hasTopWall) {
            SDL_FRect wallRect = {cellRect.x, cellRect.y, cellRect.w, wallThickness};
            SDL_RenderFillRect(renderer, &wallRect);
        }
        
        if (cell.hasBottomWall) {
            SDL_FRect wallRect = {cellRect.x, cellRect.y + cellRect.h - wallThickness, cellRect.w, wallThickness};
            SDL_RenderFillRect(renderer, &wallRect);
        }
        
        if (cell.hasLeftWall) {
            SDL_FRect wallRect = {cellRect.x, cellRect.y, wallThickness, cellRect.h};
            SDL_RenderFillRect(renderer, &wallRect);
        }
        
        if (cell.hasRightWall) {
            SDL_FRect wallRect = {cellRect.x + cellRect.w - wallThickness, cellRect.y, wallThickness, cellRect.h};
            SDL_RenderFillRect(renderer, &wallRect);
        }
    }
    
    // Reset render target
    SDL_SetRenderTarget(renderer, nullptr);
    
    SDL_Log("Generated maze texture: %dx%d", textureWidth, textureHeight);
}

// Convert base36 character to color component
Uint8 Maze::base36ToColorComponent(char base36Char, int component) const {
    int value = 0;
    
    if (base36Char >= '0' && base36Char <= '9') {
        value = base36Char - '0';
    } else if (base36Char >= 'A' && base36Char <= 'Z') {
        value = base36Char - 'A' + 10;
    } else if (base36Char >= 'a' && base36Char <= 'z') {
        value = base36Char - 'a' + 10;
    }
    
    // Create different color variations based on component and value
    switch (component) {
        case 0: // Red component
            return static_cast<Uint8>((value * 7 + 50) % 256);
        case 1: // Green component  
            return static_cast<Uint8>((value * 11 + 100) % 256);
        case 2: // Blue component
            return static_cast<Uint8>((value * 13 + 150) % 256);
        default:
            return static_cast<Uint8>(value * 7 % 256);
    }
}