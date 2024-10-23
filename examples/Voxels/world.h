#ifndef _world_h_
#define _world_h_

#include <functional>
#include <memory>
#include <mutex>

#include "map.h"

#include <MazeBuilder/maze_builder.h>

using world_func = std::function<void(int, int, int, int, Map*)>;

class world {

public:
    void create_world(int p, int q, world_func func, Map *m, int chunk_size, const std::unique_ptr<mazes::maze_builder>& mb) const noexcept;
};

#endif
