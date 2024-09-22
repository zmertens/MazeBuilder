#ifndef _world_h_
#define _world_h_

#include <functional>
#include <memory>

#include "map.h"

using world_func = std::function<void(int, int, int, int, Map*)>;

#include "maze_thread_safe.h"

class world {
public:
    void create_world(int p, int q, const std::unique_ptr<mazes::maze_thread_safe>& maze, world_func func, Map *m, int chunk_size) const noexcept;
private:
};

#endif
