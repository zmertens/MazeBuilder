#ifndef MAZE_FACTORY_ENUM_H
#define MAZE_FACTORY_ENUM_H

#include <string>

namespace mazes {
    static constexpr auto MAZE_BARRIER1 = '|';
    static constexpr auto MAZE_BARRIER2 = '-';
    static constexpr auto MAZE_CORNER = '+';

    enum class maze_types {
        BINARY_TREE = 0,
        SIDEWINDER = 1,
        DFS = 2,
        INVALID_ALGO = 3
    };

    static std::string to_string(maze_types algo) {
        switch (algo) {
        case maze_types::BINARY_TREE:
            return "binary_tree";
        case maze_types::SIDEWINDER:
            return "sidewinder";
        case maze_types::DFS:
            return "dfs";
        default:
            return "ERROR: maze type unknown";
        }
    };

    static maze_types to_maze_type(const std::string& algo) {
        if (algo.compare("binary_tree") == 0) {
            return maze_types::BINARY_TREE;
        } else if (algo.compare("sidewinder") == 0) {
            return maze_types::SIDEWINDER;
        } else if (algo.compare("dfs") == 0) {
            return maze_types::DFS;
        } else {
            return maze_types::INVALID_ALGO;
        }
    };
}

#endif // MAZE_FACTORY_ENUM_H
