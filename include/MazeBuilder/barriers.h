#ifndef BARRIERS_H
#define BARRIERS_H

#include <type_traits>

/// @namespace mazes
/// @file barriers.h
namespace mazes
{
    /// @brief Character representations of walls and barriers in the maze
    enum class barrier : char
    {
        HORIZONTAL = '-',

        VERTICAL = '|',

        CORNER = '+',

        SINGLE_SPACE = ' '
    };

    /// @brief Directional neighbors for grid topology
    enum class direction : int
    {
        NORTH = 0,
        SOUTH = 1,
        EAST = 2,
        WEST = 3
    };
} // namespace mazes

#endif // BARRIERS_H
