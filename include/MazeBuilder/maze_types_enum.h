#ifndef MAZE_FACTORY_ENUM_H
#define MAZE_FACTORY_ENUM_H

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
}

#endif // MAZE_FACTORY_ENUM_H