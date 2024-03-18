#ifndef _world_h_
#define _world_h_

#include <functional>
#include <memory>

#include "map.h"
#include "maze_algo_interface.h"
#include "binary_tree.h"

class grid;

using world_func = std::function<void(int, int, int, int, Map*)>;

class world {
public:
    void create_world(int p, int q, world_func func, Map *m);
private:
};

#endif
