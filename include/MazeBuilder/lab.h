#ifndef LAB_H
#define LAB_H

#include <memory>
#include <vector>

/// @file lab.h
/// @namespace mazes
namespace mazes
{

    class cell;
    class configurator;

    /// @class lab - abbreviation of "labyrinth"
    /// @brief Provides link operations
    /// @details This class provides methods to link and unlink cells in a maze.
    class lab
    {

    public:
        /// @brief Links two cell objects, optionally in both directions.
        /// @param c1 A shared pointer to the first cell object.
        /// @param c2 A shared pointer to the second cell object.
        /// @param bidi A boolean flag indicating if the link should be bidirectional. Defaults to true.
        static void link(const std::shared_ptr<cell> &c1, const std::shared_ptr<cell> &c2, bool bidi = true) noexcept;

        /// @brief Unlinks two cell objects, optionally in both directions.
        /// @param c1 A shared pointer to the first cell object.
        /// @param c2 A shared pointer to the second cell object.
        /// @param bidi A boolean flag indicating if the unlink should be bidirectional. Defaults to true.
        static void unlink(const std::shared_ptr<cell> &c1, const std::shared_ptr<cell> &c2, bool bidi = true) noexcept;

        /// @brief Sets neighbors for a collection of cells based on the provided indices.
        /// @param config The configurator containing maze configuration parameters.
        /// @param indices A vector of indices representing the cells to set neighbors for.
        /// @param cells_to_set A vector of cell objects to set neighbors for.
        /// @details This function uses the configurator to determine the maze structure and sets neighbors accordingly.
        static void set_neighbors(configurator const &config, const std::vector<int> &indices, std::vector<std::shared_ptr<cell>> &cells_to_set) noexcept;

    private:
    }; // class lab

}

#endif // LAB_H
