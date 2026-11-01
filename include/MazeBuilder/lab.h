#ifndef LAB_H
#define LAB_H

#include <functional>
#include <memory>

/// @file lab.h
/// @namespace mazes
namespace mazes
{
    class cell;
    class grid_operations;
    class randomizer;

    /// @class lab short-hand for "labyrinth"
    /// @brief Provides link operations and maze algorithms
    /// @details Links and unlinks cells, and carves mazes into any grid_operations implementation.
    class lab
    {
    public:
        /// @brief Links two cell objects, optionally in both directions.
        /// @param c1 A shared pointer to the first cell object.
        /// @param c2 A shared pointer to the second cell object.
        /// @param bidi A boolean flag indicating if the link should be bidirectional. Defaults to true.
        static void link(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, bool bidi = true) noexcept;

        /// @brief Unlinks two cell objects, optionally in both directions.
        /// @param c1 A shared pointer to the first cell object.
        /// @param c2 A shared pointer to the second cell object.
        /// @param bidi A boolean flag indicating if the unlink should be bidirectional. Defaults to true.
        static void unlink(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, bool bidi = true) noexcept;

        /// @brief Accepts or rejects a cell by its grid index
        using cell_filter = std::function<bool(int)>;

        /// @brief Maze algorithms sized from ops.get_dimensions().
        /// @details An optional cell_filter restricts the maze to cells whose index it accepts
        ///          (e.g. to honor a mask); an empty filter accepts every cell.
        /// @return False if the grid has no accepted cell to work on
        static bool binary_tree(grid_operations &ops, randomizer &rng, const cell_filter &open = {});
        static bool sidewinder(grid_operations &ops, randomizer &rng, const cell_filter &open = {});
        static bool dfs(grid_operations &ops, randomizer &rng, const cell_filter &open = {});
        static bool prims(grid_operations &ops, randomizer &rng, const cell_filter &open = {});
    }; // class lab
}

#endif // LAB_H
