#include "maze_factory.h"

#include <random>

#include "binary_tree.h"
#include "sidewinder.h"
#include "grid.h"

using namespace mazes;

bool maze_factory::gen_maze(maze_types maze_type, std::unique_ptr<grid>& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng) {
    switch (maze_type) {
    case maze_types::BINARY_TREE: {
        static binary_tree bt;
        return bt.run(std::ref(_grid), std::cref(get_int), std::cref(rng));
    }
    case maze_types::SIDEWINDER: {
        static sidewinder sw;
        return sw.run(std::ref(_grid), std::cref(get_int), std::cref(rng));
    }
    default:
        // Fail on unknown maze type
        return false;
    }
}

