#ifndef _world_h_
#define _world_h_

#include <functional>
#include <memory>
#include <string>

#include "map.h"

using world_func = std::function<void(int, int, int, int, Map*)>;

class world {
public:
    void create_world(int p, int q, world_func func, Map *m, const int CHUNK_SIZE, const bool SHOW_TREES, const bool SHOW_PLANTS, const bool SHOW_CLOUDS, unsigned int height, std::istringstream maze) const noexcept;
    void create_maze(int p, int q, int w, unsigned int height, world_func func, Map* m, const int CHUNK_SIZE, std::istringstream maze) const noexcept;
private:
};

#endif
