#ifndef _world_h_
#define _world_h_

#include <functional>

#include "map.h"

using world_func = std::function<void(int, int, int, int, Map*)>;

class world {
public:
    void create_world(int p, int q, bool is_part_of_maze, world_func func, Map *m,
        int chunk_size, bool show_trees, bool show_plants, bool show_clouds) const noexcept;
private:
};

#endif
