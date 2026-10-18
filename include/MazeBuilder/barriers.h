#ifndef BARRIERS_H
#define BARRIERS_H

#include <string_view>
#include <type_traits>

/// @namespace mazes
/// @file barriers.h
namespace mazes
{
    constexpr std::string_view HORIZONTAL_BARRIER_UC{"\u2500"};
    constexpr std::string_view VERTICAL_BARRIER_UC{"\u2502"};
    constexpr std::string_view CORNER_BARRIER_UC{"\u253C"};
    constexpr std::string_view SINGLE_SPACE_BARRIER_UC{"\u0020"};

    /// @brief Character representations of walls and barriers in the maze
    enum class Barrier : char
    {
        HORIZONTAL = '-',

        VERTICAL = '|',

        CORNER = '+',

        SINGLE_SPACE = ' '
    };

    /// @brief Directional neighbors for grid topology
    enum class Direction : int
    {
        NORTH = 0,
        SOUTH = 1,
        EAST = 2,
        WEST = 3
    };
} // namespace mazes

#endif // BARRIERS_H
