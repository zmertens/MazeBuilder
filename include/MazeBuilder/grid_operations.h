#ifndef GRID_OPERATIONS_H
#define GRID_OPERATIONS_H

#include <MazeBuilder/cell.h>
#include <MazeBuilder/enums.h>

#include <memory>
#include <tuple>
#include <vector>

namespace mazes {

/// @file grid_operations.h
/// @class grid_operations
/// @brief Interface for grid navigation and manipulation operations
class grid_operations {

public:

    virtual ~grid_operations() = default;

    virtual std::tuple<unsigned int, unsigned int, unsigned int> get_dimensions() const noexcept = 0;

    /// @brief Get neighbor by the cell's respective location
    /// @param c 
    /// @param dir 
    /// @return 
    virtual std::shared_ptr<cell> get_neighbor(std::shared_ptr<cell> const& c, Direction dir) const noexcept = 0;

    /// @brief Get all the neighbors by the cell
    /// @param c 
    /// @return 
    virtual std::vector<std::shared_ptr<cell>> get_neighbors(std::shared_ptr<cell> const& c) const noexcept = 0;

    /// @brief Set neighbor for a cell in a given direction
    /// @param c 
    /// @param dir 
    /// @param neighbor
    /// @return 
    virtual void set_neighbor(const std::shared_ptr<cell>& c, Direction dir, std::shared_ptr<cell> const& neighbor) noexcept = 0;

    /// @brief Transformation and display cells
    /// @param cells Vector to fill with cells
    virtual void sort(std::vector<std::shared_ptr<cell>>& cells) const noexcept = 0;

    // Convenience methods for accessing neighbors
    virtual std::shared_ptr<cell> get_north(const std::shared_ptr<cell>& c) const noexcept = 0;
    virtual std::shared_ptr<cell> get_south(const std::shared_ptr<cell>& c) const noexcept = 0;
    virtual std::shared_ptr<cell> get_east(const std::shared_ptr<cell>& c) const noexcept = 0;
    virtual std::shared_ptr<cell> get_west(const std::shared_ptr<cell>& c) const noexcept = 0;

    /// @brief Search for a cell by index
    /// @param index
    /// @return
    virtual std::shared_ptr<cell> search(int index) const noexcept = 0;

    virtual std::vector<std::shared_ptr<cell>> get_cells() const noexcept = 0;

    /// @brief Get the count of cells in the grid
    /// @return The number of cells in the grid
    virtual int num_cells() const noexcept = 0;

    /// @brief Cleanup cells by cleaning up links within cells
    virtual void clear_cells() noexcept = 0;

    /// @brief Configure the grid's cells' neighbors
    /// @param indices 
    /// @return 
    virtual void configure(const std::vector<int>& indices) noexcept = 0;

private:

    /// @brief Configure cells
    /// @param cells 
    virtual void configure_cells(std::vector<std::shared_ptr<cell>>& cells) noexcept = 0;

};

} // namespace mazes

#endif // GRID_OPERATIONS_H
