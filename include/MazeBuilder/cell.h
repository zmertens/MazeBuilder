#ifndef CELL_H
#define CELL_H

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>

#include <MazeBuilder/hash_funcs.h>

/// @file cell.h
/// @namespace mazes
namespace mazes
{
    /// @class cell
    /// @brief Cell class with links to other cells
    class cell final
    {
        /// @brief Equals function for weak pointers to cell objects.
        struct weak_ptr_equal
        {
            template <typename T>
            bool operator()(const std::weak_ptr<T> &lhs, const std::weak_ptr<T> &rhs) const
            {
                return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
            }
        };

    public:
        using cell_area = std::pair<std::uint8_t, std::uint8_t>;
        using cell_map = std::unordered_map<std::weak_ptr<cell>, bool, weak_ptr_hash, weak_ptr_equal>;

        /// @brief Constructs a cell object with an optional index.
        /// @param index The area (row, column per convention) to initialize the cell with.
        explicit cell(std::pair<std::uint8_t, std::uint8_t> index 
            = std::pair<std::uint8_t, std::uint8_t>{1u, 1u});

        /// @brief Destroys the cell object and releases any associated resources.
        ~cell();

        /// @brief Copy constructor for the cell class.
        /// @param other The cell object to copy from.
        cell(const cell &other);

        /// @brief Assigns the value of another cell to this cell.
        /// @param other The cell whose value will be assigned to this cell.
        /// @return A reference to this cell after assignment.
        cell &operator=(const cell &other);

        /// @brief Move constructor for the cell class.
        /// @param other The cell object to move from.
        cell(cell &&other) noexcept;

        /// @brief Move assignment operator for the cell class.
        /// @param other The cell object to move from.
        /// @return A reference to the assigned cell object (*this).
        cell &operator=(cell &&other) noexcept;

        /// @brief Add a link to another cell (passage between cells)
        /// @param other The cell to link to
        void add_link(const std::shared_ptr<cell> &other) noexcept;

        /// @brief Remove a link to another cell
        /// @param other The cell to unlink from
        void remove_link(const std::shared_ptr<cell> &other) noexcept;

        /// @brief Remove all links so the cell can be reused
        void clear_links() noexcept;

        /// @brief Checks if a cell is linked.
        /// @param c A shared pointer to the cell to check.
        /// @return True if the cell is linked, false otherwise.
        bool is_linked(const std::shared_ptr<cell> &c) const noexcept;

        /// @brief Retrieves the area of the current cell.
        /// @return The area of the current cell.
        cell_area get_area() const noexcept;

        /// @brief Sets the area to the specified value.
        /// @param new_area The new area value to set.
        void set_area(cell_area new_area) noexcept;

    private:
        /// @brief Removes expired links
        void cleanup_links();

        /// @brief Checks if the cell has a link to the specified cell.
        /// @param c A shared pointer to the cell to check.
        /// @return True if the cell has a link to the specified cell, false otherwise.
        bool has_key(const std::shared_ptr<cell> &c) const noexcept;

        cell_area m_area;
        cell_map m_links;
    };
} // namespace mazes

#endif // CELL_H
