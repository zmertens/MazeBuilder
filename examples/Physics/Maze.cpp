#include "Maze.hpp"

#include "OrthographicCamera.hpp"

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create.h>
#include <MazeBuilder/create2.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>

// Constructor
Maze::Maze() {
}

// Destructor  
Maze::~Maze() {
}

// Initialize maze from parsed data
bool Maze::initialize(SDL_Renderer* renderer, const std::vector<Maze::Cell>& cells, int mazeRows, int mazeCols, float cellSize) {
    if (cells.empty() || mazeRows <= 0 || mazeCols <= 0 || !renderer) {
        return false;
    }

    this->cells = cells;
    rows = mazeRows;
    cols = mazeCols;
    cellSize = cellSize;

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
        std::uint8_t red = base36ToColorComponent(cell.colorValue, 0);
        std::uint8_t green = base36ToColorComponent(cell.colorValue, 1);
        std::uint8_t blue = base36ToColorComponent(cell.colorValue, 2);
        
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
std::uint8_t Maze::base36ToColorComponent(char base36Char, int component) const {
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
            return static_cast<std::uint8_t>((value * 7 + 50) % 256);
        case 1: // Green component  
            return static_cast<std::uint8_t>((value * 11 + 100) % 256);
        case 2: // Blue component
            return static_cast<std::uint8_t>((value * 13 + 150) % 256);
        default:
            return static_cast<std::uint8_t>(value * 7 % 256);
    }
}

// Complete maze generation and initialization
bool Maze::generateAndInitializeMaze(SDL_Renderer* renderer, int displayWidth, int displayHeight) {
    std::string mazeStr = generateNewMazeString(displayWidth, displayHeight);
    if (mazeStr.empty()) {
        return false;
    }
    
    int mazeRows, mazeCols;
    auto mazeCells = parse(mazeStr, mazeRows, mazeCols);
    if (mazeCells.empty()) {
        return false;
    }
    
    float cellSize = std::min(displayWidth / (float)mazeCols, displayHeight / (float)mazeRows) * 0.8f;
    return initialize(renderer, mazeCells, mazeRows, mazeCols, cellSize);
}

std::string Maze::generateNewMazeString(int displayWidth, int displayHeight) {
    // Calculate optimal maze size based on display dimensions
    int optimalCols = std::max(10, displayWidth / 40);  // ~40 pixels per cell
    int optimalRows = std::max(8, displayHeight / 40);
    
    mazes::configurator config;
    config.rows(optimalRows)
          .columns(optimalCols)
          .distances(true)
          .distances_start(0)
          .distances_end(-1)
          .seed(static_cast<unsigned int>(SDL_GetTicks()));
    
    return mazes::create(config);
}

// Start background maze generation
void Maze::startBackgroundMazeGeneration() noexcept{

    if (mazeGenerationStarted) {

        return;
    }
    
    mazeGenerationStarted = true;
    
    // Start async maze generation
    mazeGenerationFuture = std::async(std::launch::async, [this]() -> std::vector<std::string> {
        std::vector<mazes::configurator> configs;
        
        // Create 10 different maze configurations
        for (int i = 0; i < 10; i++) {
            mazes::configurator config;
            config.rows(mazes::configurator::DEFAULT_ROWS)
                    .columns(mazes::configurator::DEFAULT_COLUMNS)
                    .distances(true)
                    .distances_start(0)
                    .distances_end(-1)
                    .seed(static_cast<unsigned int>(SDL_GetTicks() + i * 1000));
            
            configs.push_back(config);
        }
        
        // Use create2 for concurrent generation
        return {mazes::create2(configs)};
    });
    
    SDL_Log("Background maze generation started");
}

// Check if maze generation is complete and get results
bool Maze::isReady() const noexcept {

    if (!mazeGenerationStarted || !mazeGenerationFuture.valid()) {

        return false;
    }
    
    // Check if generation is complete
    return mazeGenerationFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

// Parse maze string to extract walls and cell colors

std::vector<Maze::Cell> Maze::parse(const std::string& mazeStr, int& rows, int& cols) const noexcept {
    
    std::vector<Maze::Cell> cells;

    if (mazeStr.empty()) {

        return cells;
    }
    
    // Parse the maze string to find cells and walls
    std::vector<std::string> lines;
    std::istringstream iss(mazeStr);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    
    if (lines.empty()) {
        return cells;
    }
    
    // Calculate dimensions from the maze structure
    rows = (lines.size() - 1) / 2;  // Every other line is a cell row
    cols = 0;
    
    // Find the first cell line to count columns
    for (size_t i = 1; i < lines.size(); i += 2) {
        const std::string& cellLine = lines[i];
        int colCount = 0;
        for (size_t j = 1; j < cellLine.length(); j += 2) {
            if (j < cellLine.length() && cellLine[j] != '|' && cellLine[j] != ' ') {
                colCount++;
            }
        }
        cols = std::max(cols, colCount);
        break;
    }
    
    cells.resize(rows * cols);
    
    // Parse cells and their walls
    int cellRow = 0;
    for (size_t lineIdx = 1; lineIdx < lines.size() && cellRow < rows; lineIdx += 2) {
        const std::string& cellLine = lines[lineIdx];
        const std::string& topWallLine = (lineIdx > 0) ? lines[lineIdx - 1] : "";
        const std::string& bottomWallLine = (lineIdx + 1 < lines.size()) ? lines[lineIdx + 1] : "";
        
        int cellCol = 0;
        for (size_t charIdx = 1; charIdx < cellLine.length() && cellCol < cols; charIdx += 2) {
            if (charIdx < cellLine.length()) {
                int cellIndex = cellRow * cols + cellCol;
                if (cellIndex < cells.size()) {
                    Cell& cell = cells[cellIndex];
                    cell.row = cellRow;
                    cell.col = cellCol;
                    
                    // Extract color value
                    char ch = cellLine[charIdx];
                    if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
                        cell.colorValue = ch;
                    }
                    
                    // Check walls
                    // Left wall
                    if (charIdx > 0 && cellLine[charIdx - 1] == '|') {
                        cell.hasLeftWall = true;
                    }
                    
                    // Right wall
                    if (charIdx + 1 < cellLine.length() && cellLine[charIdx + 1] == '|') {
                        cell.hasRightWall = true;
                    }
                    
                    // Top wall
                    if (!topWallLine.empty() && charIdx < topWallLine.length() && topWallLine[charIdx] == '-') {
                        cell.hasTopWall = true;
                    }
                    
                    // Bottom wall
                    if (!bottomWallLine.empty() && charIdx < bottomWallLine.length() && bottomWallLine[charIdx] == '-') {
                        cell.hasBottomWall = true;
                    }
                }
                cellCol++;
            }
        }
        cellRow++;
    }
    
    SDL_Log("Parsed maze: %dx%d with %zu cells", rows, cols, cells.size());
    return cells;
}