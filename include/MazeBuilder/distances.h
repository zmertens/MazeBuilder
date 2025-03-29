#ifndef DISTANCES_H
#define DISTANCES_H

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <memory>

namespace mazes {

class cell;

/// @file distances.h
/// @class distances
/// @brief Distances utility class for counting paths and nodes
/// @details This class is used to compute the distances between cells in a maze
class distances {
public:
    /// @brief Constructor that initializes the distances object with a given root cell.
    /// @param root A shared pointer to the root cell used to initialize the distances object.
    explicit distances(std::shared_ptr<cell> root);

    /// @brief Overloaded operator to access the distance of a cell.
    /// @param cell A shared pointer to the cell whose distance is to be accessed.
    /// @return A reference to the integer distance associated with the specified cell.
	int& operator[](const std::shared_ptr<cell>& cell) noexcept {
		return m_cells[cell];
	}

    /// @brief Accesses the value associated with a given cell in a shared pointer.
    /// @param cell A shared pointer to the cell whose associated value is to be accessed.
    /// @return A constant reference to the integer value associated with the specified cell.
    const int& operator[](const std::shared_ptr<cell>& cell) const noexcept {
        return m_cells.at(cell);
    }

    /// @brief Sets the distance of a cell in the distances object.
    /// @param cell A shared pointer to the cell whose distance is to be set.
    void set(std::shared_ptr<cell> cell, int distance) noexcept;

    /// @brief Checks if a given cell is contained in the distances object.
    /// @param cell A shared pointer to the cell to check for containment.
    bool contains(const std::shared_ptr<cell>& cell) const noexcept;

    /// @brief Computes the shortest path to a goal cell within a distances object.
    std::shared_ptr<distances> path_to(std::shared_ptr<cell> goal) const noexcept;

    /// @brief Computes the maximum distance and cell in a distances object.
    std::pair<std::shared_ptr<cell>, int> max() const noexcept;

    /// @brief Collects keys from a vector of shared pointers to cell objects.
    /// @param cells A reference to a vector of shared pointers to cell objects from which keys will be collected.
    void collect_keys(std::vector<std::shared_ptr<cell>>& cells) const noexcept;

private:
    std::shared_ptr<cell> m_root;
    std::unordered_map<std::shared_ptr<cell>, int> m_cells;
};

} // namespace mazes
#endif // DISTANCES_H
