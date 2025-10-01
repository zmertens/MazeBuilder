#ifndef GRID_RANGE_H
#define GRID_RANGE_H

#include <MazeBuilder/cell.h>

#include <functional>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace mazes
{
    /// @brief Forward declaration
    class grid;

    /// @brief Iterator for grid cells with lazy creation support
    class grid_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::shared_ptr<cell>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        /// @brief Constructor for grid iterator
        /// @param cells Reference to the cells map
        /// @param dimensions Grid dimensions for bounds checking
        /// @param current_index Current index position
        /// @param end_index End index (exclusive)
        /// @param create_cells_func Function to create cells lazily
        grid_iterator(
            std::unordered_map<int, std::shared_ptr<cell>>& cells,
            std::tuple<unsigned int, unsigned int, unsigned int> dimensions,
            int current_index,
            int end_index,
            std::function<std::shared_ptr<cell>(int)> create_cells_func = nullptr
        );

        /// @brief Dereference operator
        /// @return Shared pointer to the current cell
        value_type operator*() const;

        /// @brief Pre-increment operator
        /// @return Reference to this iterator
        grid_iterator& operator++();

        /// @brief Post-increment operator
        /// @return Copy of iterator before increment
        grid_iterator operator++(int);

        /// @brief Equality comparison
        /// @param other Other iterator to compare with
        /// @return True if iterators are equal
        bool operator==(const grid_iterator& other) const;

        /// @brief Inequality comparison
        /// @param other Other iterator to compare with
        /// @return True if iterators are not equal
        bool operator!=(const grid_iterator& other) const;

        /// @brief Get current index
        /// @return Current index
        int get_index() const noexcept { return m_current_index; }

    private:
        std::unordered_map<int, std::shared_ptr<cell>>& m_cells;
        std::tuple<unsigned int, unsigned int, unsigned int> m_dimensions;
        int m_current_index;
        int m_end_index;
        std::function<std::shared_ptr<cell>(int)> m_create_cells_func;

        /// @brief Check if index is within valid bounds
        /// @param index Index to check
        /// @return True if index is valid
        bool is_valid_index(int index) const noexcept;

        /// @brief Get or create cell at index
        /// @param index Cell index
        /// @return Shared pointer to cell
        std::shared_ptr<cell> get_or_create_cell(int index) const;
    };

    /// @brief Range class for iterating over grid cells
    class grid_range
    {
    public:
        /// @brief Constructor for full grid range
        /// @param cells Reference to the cells map
        /// @param dimensions Grid dimensions
        /// @param create_cells_func Function to create cells lazily
        grid_range(
            std::unordered_map<int, std::shared_ptr<cell>>& cells,
            std::tuple<unsigned int, unsigned int, unsigned int> dimensions,
            std::function<std::shared_ptr<cell>(int)> create_cells_func = nullptr
        );

        /// @brief Constructor for partial grid range
        /// @param cells Reference to the cells map
        /// @param dimensions Grid dimensions
        /// @param start_index Starting index (inclusive)
        /// @param end_index Ending index (exclusive)
        /// @param create_cells_func Function to create cells lazily
        grid_range(
            std::unordered_map<int, std::shared_ptr<cell>>& cells,
            std::tuple<unsigned int, unsigned int, unsigned int> dimensions,
            int start_index,
            int end_index,
            std::function<std::shared_ptr<cell>(int)> create_cells_func = nullptr
        );

        /// @brief Get begin iterator
        /// @return Begin iterator
        grid_iterator begin();

        /// @brief Get end iterator
        /// @return End iterator
        grid_iterator end();

        /// @brief Get const begin iterator
        /// @return Const begin iterator
        grid_iterator begin() const;

        /// @brief Get const end iterator
        /// @return Const end iterator
        grid_iterator end() const;

        /// @brief Get range size
        /// @return Number of cells in range
        size_t size() const noexcept;

        /// @brief Check if range is empty
        /// @return True if range is empty
        bool empty() const noexcept;

        /// @brief Convert range to vector
        /// @return Vector of cells in the range
        std::vector<std::shared_ptr<cell>> to_vector() const;

        /// @brief Clear all cells in this range
        void clear();

        /// @brief Set cells from vector (only for cells within this range)
        /// @param cells Vector of cells to set
        /// @return True if successful
        bool set_from_vector(const std::vector<std::shared_ptr<cell>>& cells);

    private:
        std::unordered_map<int, std::shared_ptr<cell>>& m_cells;
        std::tuple<unsigned int, unsigned int, unsigned int> m_dimensions;
        int m_start_index;
        int m_end_index;
        std::function<std::shared_ptr<cell>(int)> m_create_cells_func;

        /// @brief Calculate maximum valid index for the grid
        /// @return Maximum valid index
        int max_valid_index() const noexcept;
    };

} // namespace mazes

#endif // GRID_RANGE_H
