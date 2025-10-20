#include "MazeRenderer.hpp"

#include <SDL3/SDL.h>
#include <cmath>
#include <future>
#include <sstream>
#include <random>

#include <MazeBuilder/create.h>
#include <MazeBuilder/create2.h>

#include "Maze.hpp"

class MazeRenderer::MazeRendererImpl {
public:
    // Maze distance data
    std::unordered_map<int, char> distanceMap; // cell index -> base36 distance character
    SDL_Texture* mazeDistanceTexture = nullptr;
    int mazeWidth = 0, mazeHeight = 0;
    
    // Background maze generation
    std::vector<std::string> generatedMazes; // Store pre-generated mazes
    std::future<std::vector<std::string>> mazeGenerationFuture; // Async maze generation
    bool mazeGenerationStarted = false;
    
    // Rendering parameters
    float cellSize = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    
    MazeRendererImpl() = default;
    
    ~MazeRendererImpl() {
        if (mazeDistanceTexture) {
            SDL_DestroyTexture(mazeDistanceTexture);
        }
    }
    
    // Generate a maze with distance calculations using the MazeBuilder create API
    std::string generateMazeWithDistances(int rows, int cols) {
        mazes::configurator config;
        config.rows(rows)
              .columns(cols)
              .distances(true)
              .distances_start(0)
              .distances_end(-1)
              .seed(static_cast<unsigned int>(SDL_GetTicks()));
        
        return mazes::create(config);
    }
    
    // Generate a simple maze without distances
    std::string generateSimpleMaze(int rows, int cols) {
        mazes::configurator config;
        config.rows(rows)
              .columns(cols)
              .distances(false)
              .seed(static_cast<unsigned int>(SDL_GetTicks()));
        
        std::string generatedMaze = mazes::create(config);
        
        if (generatedMaze.empty()) {
            // Create a simple fallback maze
            return "+---+---+\n|   |   |\n+   +   +\n|       |\n+---+---+\n";
        }
        
        return generatedMaze;
    }
    
    // Start background maze generation
    void startBackgroundMazeGeneration(int rows, int cols, int numMazes) {
        if (mazeGenerationStarted) {
            return; // Already started
        }
        
        mazeGenerationStarted = true;
        
        // Start async maze generation
        mazeGenerationFuture = std::async(std::launch::async, [this, rows, cols, numMazes]() -> std::vector<std::string> {
            std::vector<mazes::configurator> configs;
            
            // Create different maze configurations
            for (int i = 0; i < numMazes; i++) {
                mazes::configurator config;
                config.rows(rows)
                      .columns(cols)
                      .distances(true)  // Enable distance calculations
                      .distances_start(0)
                      .distances_end(-1)
                      .seed(static_cast<unsigned int>(SDL_GetTicks() + i * 1000)); // Different seeds
                
                configs.push_back(config);
            }
            
            // Generate individual mazes
            std::vector<std::string> results;
            for (const auto& config : configs) {
                auto result = mazes::create(config);
                results.push_back(result);
            }
            return results;
        });
        
        SDL_Log("Background maze generation started");
    }
    
    // Check if maze generation is complete and get results
    bool checkMazeGeneration() {
        if (!mazeGenerationStarted || !mazeGenerationFuture.valid()) {
            return false;
        }
        
        // Check if generation is complete
        if (mazeGenerationFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            try {
                auto result = mazeGenerationFuture.get();
                if (!result.empty()) {
                    generatedMazes = result;
                    SDL_Log("Background maze generation completed with %zu mazes", generatedMazes.size());
                    return true;
                }
            } catch (const std::exception& e) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Maze generation failed: %s", e.what());
            }
        }
        
        return false; // Still generating
    }
    
    // Parse the maze string and extract base36 distance values
    void parseMazeDistances(const std::string& mazeStr) {
        distanceMap.clear();
        
        int cellIndex = 0;
        for (size_t i = 0; i < mazeStr.length(); ++i) {
            char c = mazeStr[i];
            
            // Skip newlines and structural characters
            if (c == '\n' || c == '+' || c == '-' || c == '|' || c == ' ') {
                continue;
            }
            
            // Check if it's a base36 distance character
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                distanceMap[cellIndex] = c;
                cellIndex++;
            }
        }
    }
    
    // Create SDL texture from distance map data
    void createDistanceTexture(SDL_Renderer* renderer, int display_w, int display_h) {
        if (distanceMap.empty()) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No distance data to create texture from");
            return;
        }
        
        // Clean up existing texture
        if (mazeDistanceTexture) {
            SDL_DestroyTexture(mazeDistanceTexture);
            mazeDistanceTexture = nullptr;
        }
        
        // Calculate texture dimensions based on maze grid
        int maxCellIndex = 0;
        for (const auto& [index, distance] : distanceMap) {
            maxCellIndex = std::max(maxCellIndex, index);
        }
        
        // Assume square maze for now, improve this later
        int mazeDimension = static_cast<int>(std::sqrt(maxCellIndex + 1));
        mazeWidth = mazeDimension * 50;  // 50 pixels per cell
        mazeHeight = mazeDimension * 50;
        
        // Create texture
        mazeDistanceTexture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            mazeWidth,
            mazeHeight
        );
        
        if (!mazeDistanceTexture) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create distance texture: %s", SDL_GetError());
            return;
        }
        
        // Set texture as render target and draw distance visualization
        SDL_SetRenderTarget(renderer, mazeDistanceTexture);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White background
        SDL_RenderClear(renderer);
        
        // Draw each cell with color based on distance
        int cellSize = 50;
        for (const auto& [index, distanceChar] : distanceMap) {
            int row = index / mazeDimension;
            int col = index % mazeDimension;
            
            // Convert base36 character to numeric value for color blending
            int distanceValue = 0;
            if (distanceChar >= '0' && distanceChar <= '9') {
                distanceValue = distanceChar - '0';
            } else if (distanceChar >= 'A' && distanceChar <= 'Z') {
                distanceValue = distanceChar - 'A' + 10;
            } else if (distanceChar >= 'a' && distanceChar <= 'z') {
                distanceValue = distanceChar - 'a' + 10;
            }
            
            // Create color based on distance (closer = red, farther = blue)
            Uint8 red = static_cast<Uint8>(255 - (distanceValue * 7) % 256);
            Uint8 green = static_cast<Uint8>((distanceValue * 5) % 256);
            Uint8 blue = static_cast<Uint8>((distanceValue * 9) % 256);
            
            SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
            
            SDL_FRect cellRect = {
                static_cast<float>(col * cellSize),
                static_cast<float>(row * cellSize),
                static_cast<float>(cellSize - 1),  // Leave 1 pixel border
                static_cast<float>(cellSize - 1)
            };
            
            SDL_RenderFillRect(renderer, &cellRect);
        }
        
        // Reset render target
        SDL_SetRenderTarget(renderer, nullptr);
        
        SDL_Log("Created distance texture: %dx%d", mazeWidth, mazeHeight);
    }
};

// MazeRenderer implementation
MazeRenderer::MazeRenderer() : m_impl(std::make_unique<MazeRendererImpl>()) {
}

MazeRenderer::~MazeRenderer() = default;

std::string MazeRenderer::generateNewLevel(int rows, int cols, int displayWidth, int displayHeight) {
    return m_impl->generateSimpleMaze(rows, cols);
}

std::string MazeRenderer::generateMazeWithDistances(int rows, int cols, int displayWidth, int displayHeight) {
    std::string mazeStr = m_impl->generateMazeWithDistances(rows, cols);
    
    if (!mazeStr.empty()) {
        m_impl->parseMazeDistances(mazeStr);
        SDL_Log("Generated maze with distances: %zu distance entries", m_impl->distanceMap.size());
    }
    
    return mazeStr;
}

void MazeRenderer::startBackgroundMazeGeneration(int rows, int cols, int numMazes) {
    m_impl->startBackgroundMazeGeneration(rows, cols, numMazes);
}

bool MazeRenderer::checkMazeGeneration() {
    return m_impl->checkMazeGeneration();
}

std::vector<std::string> MazeRenderer::getGeneratedMazes() const {
    return m_impl->generatedMazes;
}

void MazeRenderer::drawMaze(SDL_Renderer* renderer, const std::string_view& cells, int displayWidth, int displayHeight) {
    drawMazeWithCamera(renderer, cells, displayWidth, displayHeight, 0.0f, 0.0f, 1.0f, 0.0f);
}

void MazeRenderer::drawMazeWithCamera(SDL_Renderer* renderer, const std::string_view& cells, int displayWidth, int displayHeight, 
                                     float cameraX, float cameraY, float zoom, float rotation) {
    if (cells.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Empty maze data provided for drawing.");
        return;
    }
    
    // Calculate maze dimensions
    int maxCols = 0;
    int maxRows = 0;
    const char* mazeData = cells.data();
    size_t mazeLen = cells.size();
    int currentRow = 0;
    int colCount = 0;
    
    for (size_t i = 0; i < mazeLen; i++) {
        char c = mazeData[i];
        if (c == '\n') {
            maxCols = std::max(maxCols, colCount);
            colCount = 0;
            currentRow++;
        } else {
            colCount++;
        }
    }
    maxRows = currentRow + 1;
    
    // Reset for drawing
    currentRow = 0;
    int currentCol = 0;
    
    // Calculate cell size to fit the display with proper padding
    float cellW = static_cast<float>(displayWidth) / static_cast<float>(maxCols);
    float cellH = static_cast<float>(displayHeight) / static_cast<float>(maxRows);
    float cellSize = std::min(cellW, cellH) * 0.95f; // 5% padding for better visibility
    
    // Make sure path cells are large enough to fit balls
    float minCellSize = 20.0f; // Minimum size for comfortable navigation
    cellSize = std::max(cellSize, minCellSize);
    
    // Center the maze in the display
    float mazeWidth = maxCols * cellSize;
    float mazeHeight = maxRows * cellSize;
    float offsetX = (displayWidth - mazeWidth) / 2.0f;
    float offsetY = (displayHeight - mazeHeight) / 2.0f;
    
    // Ensure the maze is centered even if it's bigger than the display
    offsetX = std::max(0.0f, offsetX);
    offsetY = std::max(0.0f, offsetY);
    
    // Store parameters
    m_impl->cellSize = cellSize;
    m_impl->offsetX = offsetX;
    m_impl->offsetY = offsetY;
    
    // Draw maze background
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // Light background
    SDL_RenderClear(renderer);
    
    // Draw maze cells - render all characters, not just spaces
    for (size_t i = 0; i < mazeLen; i++) {
        char c = mazeData[i];
        if (c == '\n') {
            currentCol = 0;
            currentRow++;
            continue;
        }
        
        // Calculate position - apply camera transformation
        float x = offsetX + (currentCol * cellSize) + cameraX;
        float y = offsetY + (currentRow * cellSize) + cameraY;
        
        SDL_FRect rect = {x, y, cellSize * zoom, cellSize * zoom};
        
        // Set color and draw based on cell type
        switch(c) {
            case ' ': // Open path - light green
                SDL_SetRenderDrawColor(renderer, 0x90, 0xFF, 0x90, 0xFF);
                SDL_RenderFillRect(renderer, &rect);
                break;
            case '-': // Horizontal wall - red  
                SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
                SDL_RenderFillRect(renderer, &rect);
                break;
            case '|': // Vertical wall - red
                SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
                SDL_RenderFillRect(renderer, &rect);
                break;
            case '+': // Wall junction - dark red
                SDL_SetRenderDrawColor(renderer, 0x80, 0x00, 0x00, 0xFF);
                SDL_RenderFillRect(renderer, &rect);
                break;
            default:
                // Check if it's a distance character (0-9, A-Z, a-z)
                if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                    // Draw cell based on distance value - use blue tones
                    int distanceValue = 0;
                    if (c >= '0' && c <= '9') {
                        distanceValue = c - '0';
                    } else if (c >= 'A' && c <= 'Z') {
                        distanceValue = c - 'A' + 10;
                    } else if (c >= 'a' && c <= 'z') {
                        distanceValue = c - 'a' + 10;
                    }
                    
                    // Create varying blue intensity
                    uint8_t intensity = static_cast<uint8_t>(50 + (distanceValue * 8) % 200);
                    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, intensity, 0xFF);
                    SDL_RenderFillRect(renderer, &rect);
                } else {
                    // Unknown character - use yellow
                    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF);
                    SDL_RenderFillRect(renderer, &rect);
                }
                break;
        }
        
        currentCol++;
    }
}

void MazeRenderer::createDistanceTexture(SDL_Renderer* renderer, int displayWidth, int displayHeight) {
    m_impl->createDistanceTexture(renderer, displayWidth, displayHeight);
}

SDL_Texture* MazeRenderer::getDistanceTexture() const {
    return m_impl->mazeDistanceTexture;
}

std::vector<Maze> MazeRenderer::parseMazeForRendering(const std::string& mazeStr, SDL_Renderer* renderer) {
    std::vector<Maze> mazes;
    
    // This is where we would integrate with the new Maze class
    // For now, return empty vector as the Maze class integration is still in progress
    
    return mazes;
}