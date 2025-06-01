#ifndef GRID_H
#define GRID_H

#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid_interface.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace mazes {

/// @file grid.h
/// @class grid
/// @brief General purpose grid class for 2D maze generation
class grid : public grid_interface {

    /// @brief Friend classes
    friend class binary_tree;
    friend class dfs;
    friend class sidewinder;

public:

    /// @brief 
    /// @param r 
    /// @param c 
    /// @param l 
    explicit grid(unsigned int r = 1u, unsigned int c = 1u, unsigned int l = 1u);

    /// @brief 
    /// @param dimens 
    explicit grid(std::tuple<unsigned int, unsigned int, unsigned int> dimens);

    /// @brief Copy constructor
    /// @param other 
    grid(const grid& other);

    /// @brief Assignment operator
    /// @param other 
    /// @return 
    grid& operator=(const grid& other);

    /// @brief Move constructor
    /// @param other 
    grid(grid&& other) noexcept;

    /// @brief Move assignment operator
    /// @param other 
    /// @return 
    grid& operator=(grid&& other) noexcept;

    /// @brief Destructor
    ~grid() override;

    /// @brief Configure the grid's cells' neighbors
    /// @param indices 
    /// @return 
    virtual void configure(const std::vector<int>& indices) noexcept;

    /// @brief Get neighbor by the cell's respective location
    /// @param c 
    /// @param dir 
    /// @return 
    std::shared_ptr<cell> get_neighbor(std::shared_ptr<cell> const& c, Direction dir) const noexcept;

    /// @brief Get all the neighbors by the cell
    /// @param c 
    /// @return 
    std::vector<std::shared_ptr<cell>> get_neighbors(std::shared_ptr<cell> const& c) const noexcept;

    void set_neighbor(const std::shared_ptr<cell>& c, Direction dir, std::shared_ptr<cell> const& neighbor) noexcept;

    // Convenience methods for accessing neighbors
    std::shared_ptr<cell> get_north(const std::shared_ptr<cell>& c) const noexcept override;

    std::shared_ptr<cell> get_south(const std::shared_ptr<cell>& c) const noexcept override;

    std::shared_ptr<cell> get_east(const std::shared_ptr<cell>& c) const noexcept override;

    std::shared_ptr<cell> get_west(const std::shared_ptr<cell>& c) const noexcept override;

    /// @brief Provides dimensions of grid in no assumed ordering
    /// @return 
    virtual std::tuple<unsigned int, unsigned int, unsigned int> get_dimensions() const noexcept override;

    /// @brief Convert a 2D grid to a vector of cells (sorted by row then column)
    /// @param cells 
    virtual void to_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept override;

    /// @brief Get detailed information of a cell in the grid
    /// @param c 
    /// @return 
    virtual std::string contents_of(std::shared_ptr<cell>const & c) const noexcept override;

    /// @brief Get the background color for a cell in the grid
    /// @param c 
    /// @return 
    virtual std::uint32_t background_color_for(std::shared_ptr<cell> const& c) const noexcept override;

    /// @brief
    /// @param index
    /// @return
    std::shared_ptr<cell> search(int index) const noexcept;

    /// @brief Get the count of cells in the grid
    /// @return The number of cells in the grid
    int num_cells() const noexcept;

    /// @brief Cleanup cells by cleaning up links within cells
    void clear_cells() noexcept;
private:
    /// @brief Configure cells by neighbors (N, S, E, W)
    /// @param cells 
    void configure_cells(std::vector<std::shared_ptr<cell>>& cells) noexcept;

    /// @brief Calculate the flat index for a 2D grid
    std::function<int(unsigned int, unsigned int)> m_calculate_cell_index;

    // Store cells by index
    std::unordered_map<int, std::shared_ptr<cell>> m_cells;
    std::tuple<unsigned int, unsigned int, unsigned int> m_dimensions;

    // Store topology - which cell is neighbor to which in what direction
    // Key: cell index, Value: map of direction to neighbor cell index
    mutable std::mutex m_topology_mutex;
    std::unordered_map<int, std::unordered_map<Direction, int>> m_topology;

    std::atomic<bool> m_configured;
}; // class

} // namespace mazes

#endif // GRID_H
