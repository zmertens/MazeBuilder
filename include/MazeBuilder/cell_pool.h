#ifndef CELL_POOL_H
#define CELL_POOL_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

/// @file cell_pool.h
/// @namespace mazes
namespace mazes
{
    class cell;

    /// @class cell_pool
    /// @brief Reusable, lazily grown storage of cells
    class cell_pool final
    {
    public:
        cell_pool() = default;
        cell_pool(const cell_pool &) = delete;
        cell_pool &operator=(const cell_pool &) = delete;

        /// @brief Prepare rows * cols * levels cells, allocating only what the pool is missing.
        /// Every active cell is unlinked and assigned its (row, col) area.
        /// @note Invalidates spans previously returned by view()
        void acquire(std::uint8_t rows, std::uint8_t cols, std::uint8_t levels);

        /// @brief Unlink all active cells and deactivate them, keeping the allocated cells for reuse
        void release() noexcept;

        /// @brief Active cells in index order (level-major, then row, then column)
        [[nodiscard]] std::span<const std::shared_ptr<cell>> view() const noexcept;

        /// @brief Active cell at a linear index, or nullptr
        [[nodiscard]] std::shared_ptr<cell> at(int index) const noexcept;

        /// @brief Linear index of an active cell, or -1
        [[nodiscard]] int index_of(const cell *c) const noexcept;

        [[nodiscard]] std::size_t size() const noexcept { return m_active; }

        [[nodiscard]] std::size_t capacity() const noexcept { return m_cells.size(); }

    private:
        std::vector<std::shared_ptr<cell>> m_cells;
        std::unordered_map<const cell *, int> m_indices;
        std::size_t m_active{0};
    };
} // namespace mazes

#endif // CELL_POOL_H
