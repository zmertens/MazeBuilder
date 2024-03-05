#ifndef _world_h_
#define _world_h_

#include <functional>

#include "grid.h"
#include "map.h"

using world_func = std::function<void(int, int, int, int, Map*)>;

class world {
public:
    static void create_world(int p, int q, world_func func, Map *m);
private:

};

#endif
