#ifndef CELL_H
#define CELL_H

#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include <MazeBuilder/hash_funcs.h>

namespace mazes {

/// @file cell.h
/// @class cell
/// @brief Cell class with links to other cells
class cell final {
public:
    /// @brief Constructs a cell object with an optional index.
    /// @param index The index to initialize the cell with. Defaults to 0.
    explicit cell(std::int32_t index = 0);

    /// @brief Destroys the cell object and releases any associated resources.
    ~cell();

    /// @brief Copy constructor for the cell class.
    /// @param other The cell object to copy from.
    cell(const cell& other);

    /// @brief Assigns the value of another cell to this cell.
    /// @param other The cell whose value will be assigned to this cell.
    /// @return A reference to this cell after assignment.
    cell& operator=(const cell& other);

    /// @brief Move constructor for the cell class. Transfers the resources from another cell to this one.
    /// @param other The cell object to move from.
    cell(cell&& other) noexcept;

    /// @brief Move assignment operator for the cell class.
    /// @param other The cell object to move from.
    /// @return A reference to the assigned cell object (*this).
    cell& operator=(cell&& other) noexcept;

    /// @brief Add a link to another cell (passage between cells)
    /// @param other The cell to link to
    void add_link(const std::shared_ptr<cell>& other);

    /// @brief Remove a link to another cell
    /// @param other The cell to unlink from
    void remove_link(const std::shared_ptr<cell>& other);

    /// @brief Retrieves links to other cells
    /// @return A vector of pairs containing linked cells and their link status
    std::vector<std::pair<std::shared_ptr<cell>, bool>> get_links() const;

    /// @brief Checks if a cell is linked.
    /// @param c A shared pointer to the cell to check.
    /// @return True if the cell is linked, false otherwise.
    bool is_linked(const std::shared_ptr<cell>& c);

    /// @brief Retrieves the index of the current cell.
    /// @return The index of the current cell.
    int32_t get_index() const noexcept;

    /// @brief Sets the index to the specified value.
    /// @param next_index The new index value to set.
    void set_index(std::int32_t next_index) noexcept;

    /// @brief Cleans up or removes links, typically as part of a resource management or shutdown process.
    void cleanup_links();

private:

    /// @brief Equals function for weak pointers to cell objects.
    struct weak_ptr_equal {

        template <typename T>
        bool operator()(const std::weak_ptr<T>& lhs, const std::weak_ptr<T>& rhs) const {

            return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
        }
    };

    bool has_key(const std::shared_ptr<cell>& c);

    std::unordered_map<std::weak_ptr<cell>, bool, weak_ptr_hash, weak_ptr_equal> m_links;

    mutable std::shared_mutex m_links_mutex;

    std::int32_t m_index;
};

} // namespace mazes

#endif // CELL_H
