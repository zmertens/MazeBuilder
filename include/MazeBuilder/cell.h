#ifndef CELL_H
#define CELL_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace mazes {

/// @file cell.h
/// @class cell
/// @brief Cell class for maze generation
class cell : public std::enable_shared_from_this<cell> {

public:
    /// @brief Constructs a cell object with an optional index.
    /// @param index The index to initialize the cell with. Defaults to 0.
    explicit cell(std::int32_t index = 0);

    /// @brief Links two cell objects, optionally in both directions.
    /// @param c1 A shared pointer to the first cell object.
    /// @param c2 A shared pointer to the second cell object.
    /// @param bidi A boolean flag indicating if the link should be bidirectional. Defaults to true.
    void link(const std::shared_ptr<cell>& other, bool bidi = true) noexcept;

    /// @brief Unlinks two cells, optionally in both directions.
    /// @param c1 A shared pointer to the first cell.
    /// @param c2 A shared pointer to the second cell.
    /// @param bidi A boolean indicating whether to unlink in both directions. Defaults to true.
    void unlink(const std::shared_ptr<cell>& other, bool bidi = true) noexcept;

    /// @brief Retrieves a constant reference to an unordered map of cell links and their boolean states.
    /// @return A constant reference to an unordered map where the keys are shared pointers to cell objects
    ///     and the values are boolean states indicating the link status.
    const std::unordered_map<std::shared_ptr<cell>, bool>& get_links();

    /// @brief Checks if a cell is linked.
    /// @param c A shared pointer to the cell to check.
    /// @return True if the cell is linked, false otherwise.
    bool is_linked(const std::shared_ptr<cell>& c);

    /// @brief Checks if the current object has a northern neighbor.
    /// @return A boolean value indicating whether the current object has a northern neighbor.
    bool has_northern_neighbor() const noexcept;

    /// @brief Checks if the current object has a southern neighbor.
    /// @return A boolean value indicating whether the current object has a southern neighbor.
    bool has_southern_neighbor() const noexcept;

    /// @brief Checks if there is an eastern neighbor.
    /// @return A boolean value indicating whether there is an eastern neighbor.
    bool has_eastern_neighbor() const noexcept;

    /// @brief Checks if there is a western neighbor.
    /// @return A boolean value indicating whether there is a western neighbor.
    bool has_western_neighbor() const noexcept;

    /// @brief Retrieves a list of neighboring cells.
    /// @return A vector of shared pointers to neighboring cells.
    std::vector<std::shared_ptr<cell>> get_neighbors() const noexcept;

    /// @brief Retrieves the index of the current cell.
    /// @return The index of the current cell.
    int32_t get_index() const noexcept;

	/// @brief Sets the index to the specified value.
	/// @param next_index The new index value to set.
	void set_index(std::int32_t next_index) noexcept;

    /// @brief Retrieves a shared pointer to the cell located to the north.
    /// @return A shared pointer to the cell located to the north.
    std::shared_ptr<cell> get_north() const;

    /// @brief Retrieves a shared pointer to the cell located to the south.
    /// @return A shared pointer to the cell located to the south.
    std::shared_ptr<cell> get_south() const;

    /// @brief Retrieves a shared pointer to the cell located to the east.
    /// @return A std::shared_ptr pointing to the cell located to the east.
    std::shared_ptr<cell> get_east() const;

    /// @brief Retrieves a shared pointer to the cell located to the west.
    /// @return A shared pointer to the cell located to the west.
    std::shared_ptr<cell> get_west() const;

    /// @brief Sets the northern neighbor of the current cell.
    /// @param other A shared pointer to the cell that will be set as the northern neighbor.
    void set_north(std::shared_ptr<cell> const& other);

    /// @brief Sets the southern neighbor of the current cell.
    /// @param other A shared pointer to the cell that will be set as the southern neighbor.
    void set_south(std::shared_ptr<cell> const& other);

    /// @brief Sets the east neighbor of the current cell.
    /// @param other A shared pointer to the cell that will be set as the east neighbor.
    void set_east(std::shared_ptr<cell> const& other);

    /// @brief Sets the west neighbor of the current cell.
    /// @param other A shared pointer to the cell that will be set as the west neighbor.
    void set_west(std::shared_ptr<cell> const& other);
private:
    bool has_key(const std::shared_ptr<cell>& c);

    std::unordered_map<std::shared_ptr<cell>, bool> m_links;

    std::int32_t m_index;

    std::shared_ptr<cell> m_north;
    std::shared_ptr<cell> m_south;
    std::shared_ptr<cell> m_east;
    std::shared_ptr<cell> m_west;
}; // class

} // namespace

#endif // CELL_H
