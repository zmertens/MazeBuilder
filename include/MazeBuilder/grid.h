#ifndef GRID_H
#define GRID_H

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

// Define direction enum for grid topology
enum class Direction : std::uint8_t {
    North = 0,
    South = 1,
    East = 2,
    West = 3,
    Count // Used to determine array size
};

/// @file grid.h
/// @class grid
/// @brief General purpose grid class for maze generation
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

    /// @brief 
    /// @param indices 
    /// @return 
    virtual void start_configuration(const std::vector<int>& indices) noexcept;

    // Get and set neighbors for a cell
    std::shared_ptr<cell> get_neighbor(const std::shared_ptr<cell>& c, Direction dir) const noexcept;

    void set_neighbor(const std::shared_ptr<cell>& c, Direction dir, const std::shared_ptr<cell>& neighbor) noexcept;

    // Convenience methods for accessing neighbors
    std::shared_ptr<cell> get_north(const std::shared_ptr<cell>& c) const noexcept {
        return get_neighbor(c, Direction::North);
    }

    std::shared_ptr<cell> get_south(const std::shared_ptr<cell>& c) const noexcept override {
        return get_neighbor(c, Direction::South);
    }

    std::shared_ptr<cell> get_east(const std::shared_ptr<cell>& c) const noexcept override {
        return get_neighbor(c, Direction::East);
    }

    std::shared_ptr<cell> get_west(const std::shared_ptr<cell>& c) const noexcept {
        return get_neighbor(c, Direction::West);
    }

    // Get all neighbors for a cell
    std::vector<std::shared_ptr<cell>> get_neighbors(const std::shared_ptr<cell>& c) const noexcept;

    /// @brief Provides dimensions of grid in no assumed ordering
    /// @return 
    virtual std::tuple<unsigned int, unsigned int, unsigned int> get_dimensions() const noexcept override;

    /// @brief Convert a 2D grid to a vector of cells (sorted by row then column)
    /// @param cells 
    virtual void to_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept override;

    /// @brief Get detailed information of a cell in the grid
    /// @param c 
    /// @return 
    virtual std::optional<std::string> contents_of(const std::shared_ptr<cell>& c) const noexcept override;

    /// @brief Get the background color for a cell in the grid
    /// @param c 
    /// @return 
    virtual std::optional<std::uint32_t> background_color_for(const std::shared_ptr<cell>& c) const noexcept override;

    /// @brief
    /// @param index
    /// @return
    std::shared_ptr<cell> search(int index) const noexcept;

    /// @brief Get the count of cells in the grid
    /// @return The number of cells in the grid
    int num_cells() const noexcept;


    /// @brief Clear cells, resetting neighbors and links
    void clear_cells() noexcept;
private:
    /// @brief Configure cells by neighbors (N, S, E, W)
    /// @param cells 
    void configure_cells(std::vector<std::shared_ptr<cell>>& cells) noexcept;

    /// @brief Calculate the flat index for a 2D grid
    std::function<int(unsigned int, unsigned int)> m_calc_index;

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
