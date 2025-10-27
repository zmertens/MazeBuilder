#include "Renderer.hpp"

#include <SDL3/SDL.h>
#include <cmath>
#include <future>
#include <sstream>
#include <random>

#include <MazeBuilder/create.h>
#include <MazeBuilder/create2.h>


struct Renderer::RendererImpl {
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
    
    RendererImpl() = default;
    
    ~RendererImpl() {
        if (mazeDistanceTexture) {
            SDL_DestroyTexture(mazeDistanceTexture);
        }
    }
    
   
};

// Renderer implementation
Renderer::Renderer() : m_impl(std::make_unique<RendererImpl>()) {
}

Renderer::~Renderer() = default;

