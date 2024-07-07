#include "maze_factory.h"

#include "binary_tree.h"
#include "sidewinder.h"
#include "grid.h"

using namespace mazes;

bool maze_factory::gen_maze(maze_types maze_type, std::unique_ptr<grid>& _grid, std::function<int(int, int)> get_int) {
    switch (maze_type) {
    case maze_types::BINARY_TREE: {
        static binary_tree bt;
        return bt.run(std::ref(_grid), get_int);
    }
    case maze_types::SIDEWINDER: {
        static sidewinder sw;
        return sw.run(std::ref(_grid), get_int);
    }
    default:
        // Fail on unknown maze type
        return false;
    }
}

