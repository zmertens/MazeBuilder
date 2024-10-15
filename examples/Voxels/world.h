#ifndef _world_h_
#define _world_h_

#include <functional>
#include <memory>
#include <mutex>

#include "map.h"

using world_func = std::function<void(int, int, int, int, Map*)>;

#include <MazeBuilder/maze_builder.h>

class world {
public:
    void create_world(int p, int q, const std::unique_ptr<mazes::maze_builder>& maze, world_func func, Map *m, int chunk_size) noexcept;
private:
    std::mutex maze_mutex;
};

#endif
