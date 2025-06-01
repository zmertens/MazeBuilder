#ifndef COLORED_GRID_H
#define COLORED_GRID_H

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <MazeBuilder/grid.h>

namespace mazes {

class cell;
class distances;

/// @file colored_grid.h
/// @class colored_grid
/// @brief Extension of the grid class to include color information
class colored_grid : public grid {

    friend class binary_tree;
    friend class dfs;
    friend class sidewinder;

public:

    /// @brief Constructs a colored grid with specified dimensions.
    /// @param width The width of the grid. Defaults to 1.
    /// @param length The length of the grid. Defaults to 1.
    /// @param levels The number of levels in the grid. Defaults to 1.
    explicit colored_grid(unsigned int width = 1u, unsigned int length = 1u, unsigned int levels = 1u);

    void configure(const std::vector<int>& indices) noexcept override;

    /// @brief Retrieves the contents of a given cell, if available.
    /// @param c A shared pointer to the cell whose contents are to be retrieved.
    /// @return An optional string containing the contents of the cell. If the cell has no contents, the optional will be empty.
    virtual std::string contents_of(const std::shared_ptr<cell>& c) const noexcept override;

    ///// @brief Retrieves the background color for a given cell, if available.
    ///// @param c A shared pointer to the cell for which the background color is to be retrieved.
    ///// @return An optional containing the background color as a 32-bit unsigned integer, or an empty optional if no background color is available.
    virtual std::uint32_t background_color_for(const std::shared_ptr<cell>& c) const noexcept override;
	
private:
    std::shared_ptr<distances> m_distances;
};

}
#endif // COLORED_GRID_H
