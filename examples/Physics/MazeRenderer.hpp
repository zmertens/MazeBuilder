#ifndef MAZE_RENDERER_HPP
#define MAZE_RENDERER_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <future>
#include <unordered_map>

struct SDL_Renderer;
struct SDL_Texture;
class Maze;

class MazeRenderer {
public:
    MazeRenderer();
    ~MazeRenderer();
    
    // Maze generation
    std::string generateNewLevel(int rows, int cols, int displayWidth, int displayHeight);
    std::string generateMazeWithDistances(int rows, int cols, int displayWidth, int displayHeight);
    
    // Background maze generation
    void startBackgroundMazeGeneration(int rows, int cols, int numMazes = 10);
    bool checkMazeGeneration();
    std::vector<std::string> getGeneratedMazes() const;
    
    // Maze rendering
    void drawMaze(SDL_Renderer* renderer, const std::string_view& cells, int displayWidth, int displayHeight);
    void drawMazeWithCamera(SDL_Renderer* renderer, const std::string_view& cells, int displayWidth, int displayHeight, 
                           float cameraX, float cameraY, float zoom, float rotation);
    
    // Distance texture management
    void createDistanceTexture(SDL_Renderer* renderer, int displayWidth, int displayHeight);
    SDL_Texture* getDistanceTexture() const;
    
    // Maze parsing for new Maze class integration
    std::vector<Maze> parseMazeForRendering(const std::string& mazeStr, SDL_Renderer* renderer);
    
private:
    struct MazeRendererImpl;
    std::unique_ptr<MazeRendererImpl> m_impl;
};

#endif // MAZE_RENDERER_HPP