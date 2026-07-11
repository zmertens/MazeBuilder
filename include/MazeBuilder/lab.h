#ifndef LAB_H
#define LAB_H

#include <memory>

/// @file lab.h
/// @namespace mazes
namespace mazes
{
    class cell;

    /// @class lab short-hand for "labyrinth"
    /// @brief Provides link operations
    /// @details This class provides methods to link and unlink cells in a maze.
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
    }; // class lab
}

#endif // LAB_H
