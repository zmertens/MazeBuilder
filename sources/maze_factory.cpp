#include <MazeBuilder/maze_factory.h>

#include <random>

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/maze_types_enum.h>

using namespace mazes;

bool maze_factory::gen(maze_types maze_type, std::unique_ptr<grid_interface>& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng) {
    switch (maze_type) {
    case maze_types::BINARY_TREE: {
        static binary_tree bt;
        return bt.run(std::ref(_grid), std::cref(get_int), std::cref(rng));
    }
    case maze_types::SIDEWINDER: {
        static sidewinder sw;
        return sw.run(std::ref(_grid), std::cref(get_int), std::cref(rng));
    }
    case maze_types::DFS: {
        static dfs d;
        return d.run(std::ref(_grid), std::cref(get_int), std::cref(rng));
    }
    default:
        // Fail on unknown maze type
        return false;
    }
}

