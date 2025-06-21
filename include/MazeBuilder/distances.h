#ifndef DISTANCES_H
#define DISTANCES_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mazes {

class grid_interface;

/// @file distances.h
/// @class distances
/// @brief A class that manages distances associated with cells in a grid.
/// @details This class provides functionality to initialize distances from a root cell,
class distances {

public:

    /// @brief Constructor that initializes the distances object with a given root index.
    /// @param root_index The index of the root cell used to initialize the distances object.
    explicit distances(int32_t root_index);

    /// @brief Overloaded operator to access the distance of a cell by index.
    /// @param index The index of the cell whose distance is to be accessed.
    /// @return A reference to the integer distance associated with the specified cell index.
    int& operator[](int32_t index) noexcept;

    /// @brief Accesses the value associated with a given cell index.
    /// @param index The index of the cell whose associated value is to be accessed.
    /// @return A constant reference to the integer value associated with the specified cell index.
    const int& operator[](int32_t index) const noexcept;

    /// @brief Sets the distance of a cell by index.
    /// @param index The index of the cell whose distance is to be set.
    void set(int32_t index, int distance) noexcept;

    /// @brief Checks if a given cell index is contained in the distances object.
    /// @param index The index of the cell to check for containment.
    bool contains(int32_t index) const noexcept;

    /// @brief Computes the shortest path to a goal cell index within a distances object.
    /// @param goal_index The index of the goal cell.
    /// @param grid A reference to the grid object for retrieving cell pointers.
    /// @return A shared pointer to a distances object representing the path.
    std::shared_ptr<distances> path_to(std::unique_ptr<grid_interface> const& g, int32_t goal_index) const noexcept;

    /// @brief Computes the maximum distance and cell index in a distances object.
    /// @return A pair containing the index of the cell with the maximum distance and the distance value.
    std::pair<int32_t, int> max() const noexcept;

    /// @brief Collects all cell indices stored in the distances object.
    /// @param indices A reference to a vector to store the collected indices.
    void collect_keys(std::vector<int32_t>& indices) const noexcept;

private:

    std::unordered_map<int32_t, int> m_cells;

    int32_t m_root_index;
}; // class distances

} // namespace mazes

#endif // DISTANCES_H
