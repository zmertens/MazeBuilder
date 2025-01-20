#ifndef ENUMS
#define ENUMS

#include <string>

namespace mazes {
    static constexpr auto MAZE_BARRIER1 = '|';
    static constexpr auto MAZE_BARRIER2 = '-';
    static constexpr auto MAZE_CORNER = '+';

    enum class outputs {
        PLAIN_TEXT,
        WAVEFRONT_OBJ_FILE,
        PNG,
        STDOUT,
        UNKNOWN
    };

    /// @brief Enum class for maze types by the generating algorithm
    enum class algos {
        BINARY_TREE = 0,
        SIDEWINDER = 1,
        DFS = 2,
        INVALID_ALGO = 3
    };

    static std::string to_string(algos algo) {
        switch (algo) {
        case algos::BINARY_TREE:
            return "binary_tree";
        case algos::SIDEWINDER:
            return "sidewinder";
        case algos::DFS:
            return "dfs";
        default:
            return std::to_string(static_cast<int>(algo));
        }
    };

    static algos to_maze_type(const std::string& algo) {
        if (algo.compare("binary_tree") == 0) {
            return algos::BINARY_TREE;
        } else if (algo.compare("sidewinder") == 0) {
            return algos::SIDEWINDER;
        } else if (algo.compare("dfs") == 0) {
            return algos::DFS;
        } else {
            return algos::INVALID_ALGO;
        }
    };
}

#endif // ENUMS
