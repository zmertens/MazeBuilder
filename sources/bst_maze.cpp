#include "bst_maze.h"

#include <SDL3/SDL.h>

bst_maze::bst_maze(const std::string& desc, unsigned int seed, const std::string& out)
: writer(out) {

}

bool bst_maze::run() {
    using namespace std;
    
#if defined(DEBUGGING)
    double start = SDL_GetTicks() / 1000.0;
    double elapsed = SDL_GetTicks() / 1000.0 - start;
    SDL_Log("Running bst algo, elapsed %d\n", elapsed);
#endif
    return true;
}