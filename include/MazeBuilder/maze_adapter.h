#ifndef MAZE_ADAPTER_H
#define MAZE_ADAPTER_H

#include <algorithm>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace mazes
{

    class cell;

    /// @file maze_adapter.h
    /// @class maze_adapter
    /// @brief Iterator adapter class for maze cell containers
    /// @details Provides a view into a maze container with string_view-like semantics
    /// Acts as a wrapper around cell containers for efficient iteration and searching
    class maze_adapter
    {
    public:
        // Type aliases for STL compatibility
        using value_type = std::shared_ptr<cell>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = value_type *;
        using const_pointer = const value_type *;

        // Container type
        using container_type = std::vector<value_type>;

        // Iterator types
        using iterator = container_type::iterator;
        using const_iterator = container_type::const_iterator;
        using reverse_iterator = container_type::reverse_iterator;
        using const_reverse_iterator = container_type::const_reverse_iterator;

        /// @brief Default constructor - creates empty adapter
        maze_adapter() noexcept = default;

        /// @brief Construct from vector of cells
        /// @param cells Vector of cell shared pointers
        explicit maze_adapter(const container_type &cells) noexcept;

        /// @brief Construct from vector of cells (move constructor)
        /// @param cells Vector of cell shared pointers
        explicit maze_adapter(container_type &&cells) noexcept;

        /// @brief Construct from iterators
        /// @param first Beginning iterator
        /// @param last End iterator
        template <typename InputIt>
        maze_adapter(InputIt first, InputIt last);

        /// @brief Copy constructor
        maze_adapter(const maze_adapter &other) = default;

        /// @brief Move constructor
        maze_adapter(maze_adapter &&other) noexcept = default;

        /// @brief Copy assignment operator
        maze_adapter &operator=(const maze_adapter &other) = default;

        /// @brief Move assignment operator
        maze_adapter &operator=(maze_adapter &&other) noexcept = default;

        /// @brief Destructor
        ~maze_adapter() = default;

        // Element access
        /// @brief Access element at index with bounds checking
        /// @param index Index of element to access
        /// @return Reference to element at index
        /// @throws std::out_of_range if index is out of bounds
        const_reference at(size_type index) const;

        /// @brief Access element at index without bounds checking
        /// @param index Index of element to access
        /// @return Reference to element at index
        const_reference operator[](size_type index) const noexcept;

        /// @brief Access first element
        /// @return Reference to first element
        /// @throws std::runtime_error if container is empty
        const_reference front() const;

        /// @brief Access last element
        /// @return Reference to last element
        /// @throws std::runtime_error if container is empty
        const_reference back() const;

        /// @brief Get pointer to underlying data
        /// @return Pointer to underlying data array
        const_pointer data() const noexcept;

        // Iterators
        /// @brief Get const iterator to beginning
        /// @return Const iterator to beginning
        const_iterator begin() const noexcept;

        /// @brief Get const iterator to beginning
        /// @return Const iterator to beginning
        const_iterator cbegin() const noexcept;

        /// @brief Get const iterator to beginning of range
        /// @param start_index Starting index (inclusive)
        /// @param count Number of elements
        /// @return Const iterator to beginning of range
        /// @throws std::out_of_range if range is invalid
        const_iterator cbegin(size_type start_index, size_type count) const;

        /// @brief Get const iterator to end
        /// @return Const iterator to end
        const_iterator end() const noexcept;

        /// @brief Get const iterator to end
        /// @return Const iterator to end
        const_iterator cend() const noexcept;

        /// @brief Get const reverse iterator to beginning
        /// @return Const reverse iterator to beginning
        const_reverse_iterator rbegin() const noexcept;

        /// @brief Get const reverse iterator to beginning
        /// @return Const reverse iterator to beginning
        const_reverse_iterator crbegin() const noexcept;

        /// @brief Get const reverse iterator to end
        /// @return Const reverse iterator to end
        const_reverse_iterator rend() const noexcept;

        /// @brief Get const reverse iterator to end
        /// @return Const reverse iterator to end
        const_reverse_iterator crend() const noexcept;

        // Capacity
        /// @brief Check if container is empty
        /// @return True if container is empty
        bool empty() const noexcept;

        /// @brief Get number of elements
        /// @return Number of elements
        size_type size() const noexcept;

        /// @brief Get maximum possible number of elements
        /// @return Maximum possible number of elements
        size_type max_size() const noexcept;

        // Search operations (string_view-like)
        /// @brief Find cell by index
        /// @param index Index to search for
        /// @return Iterator to found element, or end() if not found
        const_iterator find(int32_t index) const noexcept;

        /// @brief Find first cell that satisfies predicate
        /// @param predicate Predicate function
        /// @return Iterator to found element, or end() if not found
        template <typename UnaryPredicate>
        const_iterator find_if(UnaryPredicate predicate) const;

        /// @brief Count cells with specific index
        /// @param index Index to count
        /// @return Number of cells with the specified index
        size_type count(int32_t index) const noexcept;

        /// @brief Count cells that satisfy predicate
        /// @param predicate Predicate function
        /// @return Number of cells that satisfy predicate
        template <typename UnaryPredicate>
        size_type count_if(UnaryPredicate predicate) const;

        /// @brief Check if any cell has specific index
        /// @param index Index to check for
        /// @return True if any cell has the specified index
        bool contains(int32_t index) const noexcept;

        // Subview operations (string_view-like)
        /// @brief Create subview starting at position
        /// @param pos Starting position
        /// @return New maze_adapter representing subview
        /// @throws std::out_of_range if pos is out of bounds
        maze_adapter substr(size_type pos) const;

        /// @brief Create subview starting at position with length
        /// @param pos Starting position
        /// @param len Length of subview
        /// @return New maze_adapter representing subview
        /// @throws std::out_of_range if pos is out of bounds
        maze_adapter substr(size_type pos, size_type len) const;

        // Utility operations
        /// @brief Remove null cell pointers
        /// @return New maze_adapter with null pointers removed
        maze_adapter remove_nulls() const;

        /// @brief Sort cells by index
        /// @return New maze_adapter with cells sorted by index
        maze_adapter sort_by_index() const;

        /// @brief Get indices of all cells
        /// @return Vector of indices
        std::vector<int32_t> get_indices() const;

        // Comparison operators
        /// @brief Equality comparison
        bool operator==(const maze_adapter &other) const noexcept;

        /// @brief Inequality comparison
        bool operator!=(const maze_adapter &other) const noexcept;

    private:
        container_type m_cells;

        /// @brief Validate range parameters
        /// @param pos Starting position
        /// @param len Length (optional)
        void validate_range(size_type pos, size_type len = 0) const;
    };

    // Template implementations
    template <typename InputIt>
    maze_adapter::maze_adapter(InputIt first, InputIt last)
        : m_cells(first, last)
    {
    }

    template <typename UnaryPredicate>
    maze_adapter::const_iterator maze_adapter::find_if(UnaryPredicate predicate) const
    {
        return std::find_if(m_cells.begin(), m_cells.end(), predicate);
    }

    template <typename UnaryPredicate>
    maze_adapter::size_type maze_adapter::count_if(UnaryPredicate predicate) const
    {
        return std::count_if(m_cells.begin(), m_cells.end(), predicate);
    }

} // namespace mazes

#endif // MAZE_ADAPTER_H
