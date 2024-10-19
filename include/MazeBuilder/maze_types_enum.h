#ifndef MAZE_FACTORY_ENUM_H
#define MAZE_FACTORY_ENUM_H

#include <string>

namespace mazes {
    static constexpr auto MAZE_BARRIER1 = '|';
    static constexpr auto MAZE_BARRIER2 = '-';
    static constexpr auto MAZE_CORNER = '+';

    enum class maze_types {
        BINARY_TREE,
        SIDEWINDER,
        DFS,
        WILSONS,
        ALDOUS_BRODER,
        INVALID_ALGO
    };

    static std::string to_string(maze_types algo) {
        switch (algo) {
        case maze_types::BINARY_TREE:
            return "binary_tree";
        case maze_types::SIDEWINDER:
            return "sidewinder";
        case maze_types::DFS:
            return "dfs";
        case maze_types::WILSONS:
            return "wilsons";
        case maze_types::ALDOUS_BRODER:
            return "aldous_broder";
        default:
            return "ERROR: maze type unknown";
        }
    };
}

#endif // MAZE_FACTORY_ENUM_H