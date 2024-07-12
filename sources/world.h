#ifndef _world_h_
#define _world_h_

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <iterator>

#include "map.h"

using world_func = std::function<void(int, int, int, int, Map*)>;

class world {
public:
    void create_world(int p, int q, world_func func, Map *m, const int CHUNK_SIZE, const bool SHOW_TREES, 
        const bool SHOW_PLANTS, const bool SHOW_CLOUDS) const noexcept;
private:
};

#endif
