#ifndef CELL_FACTORY_H
#define CELL_FACTORY_H

#include <MazeBuilder/cell.h>
#include <MazeBuilder/enums.h>

#include <memory>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <functional>

namespace mazes {

/// @class cell_factory
/// @brief Service class for creating and configuring cells
class cell_factory {
public:
    /// @brief Default constructor
    cell_factory() = default;

    /// @brief Create cells based on grid dimensions
    /// @param dimensions Tuple containing (rows, columns, levels)
    /// @return Vector of created cells
    std::vector<std::shared_ptr<cell>> create_cells(
        const std::tuple<unsigned int, unsigned int, unsigned int>& dimensions) const noexcept;
    
    /// @brief Create cells based on individual dimensions
    /// @param rows Number of rows
    /// @param columns Number of columns
    /// @param levels Number of levels
    /// @return Vector of created cells
    std::vector<std::shared_ptr<cell>> create_cells(
        unsigned int rows, unsigned int columns, unsigned int levels = 1) const noexcept;

    /// @brief Configure cells with neighbors based on topology and random indices
    /// @param cells Vector of cells to configure
    /// @param dimensions Grid dimensions
    /// @param indices Optional vector of randomized indices
    void configure(
        std::vector<std::shared_ptr<cell>>& cells,
        const std::tuple<unsigned int, unsigned int, unsigned int>& dimensions,
        const std::vector<int>& indices = {}) const noexcept;

    /// @brief Create a map of cell index to cell
    /// @param cells Vector of cells
    /// @return Unordered map with cell index as key and cell as value
    std::unordered_map<int, std::shared_ptr<cell>> create_cell_map(
        const std::vector<std::shared_ptr<cell>>& cells) const noexcept;

    /// @brief Create topology map for cells
    /// @param cells Vector of cells
    /// @param dimensions Grid dimensions
    /// @return Map of cell index to direction-neighbor pairs
    std::unordered_map<int, std::unordered_map<Direction, int>> create_topology(
        const std::vector<std::shared_ptr<cell>>& cells,
        const std::tuple<unsigned int, unsigned int, unsigned int>& dimensions) const noexcept;

private:
    /// @brief Calculate cell index for a given position in the grid
    /// @param row Row position
    /// @param col Column position
    /// @param level Level position
    /// @param dimensions Grid dimensions
    /// @return Calculated cell index
    int calculate_cell_index(
        unsigned int row, unsigned int col, unsigned int level,
        const std::tuple<unsigned int, unsigned int, unsigned int>& dimensions) const noexcept;

    /// @brief Set neighbor relationships between cells
    /// @param cells Vector of cells
    /// @param cell_map Map of cell index to cell
    /// @param topology Map of cell index to direction-neighbor pairs
    void set_cell_neighbors(
        std::vector<std::shared_ptr<cell>>& cells,
        const std::unordered_map<int, std::shared_ptr<cell>>& cell_map,
        const std::unordered_map<int, std::unordered_map<Direction, int>>& topology) const noexcept;

    /// @brief Get the topology that was last created
    /// @return Map of cell index to direction-neighbor pairs
    const std::unordered_map<int, std::unordered_map<Direction, int>>& get_topology() const noexcept;

private:
    // Store the last created topology for retrieval by the grid
    mutable std::unordered_map<int, std::unordered_map<Direction, int>> m_topology;
};

} // namespace mazes

#endif // CELL_FACTORY_H
