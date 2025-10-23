#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct SDL_Renderer;
struct SDL_Texture;
class Maze;

class Renderer {
public:
    Renderer();
    ~Renderer();
    
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
    struct RendererImpl;
    std::unique_ptr<RendererImpl> m_impl;
};

#endif // RENDERER_HPP