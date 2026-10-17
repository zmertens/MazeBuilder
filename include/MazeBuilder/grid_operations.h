#ifndef GRID_OPERATIONS_H
#define GRID_OPERATIONS_H

#include <MazeBuilder/barriers.h>
#include <MazeBuilder/cell.h>

#include <cstdint>
#include <array>
#include <iterator>
#include <memory>
#include <span>
#include <tuple>
#include <vector>

/// @file grid_operations.h
/// @namespace mazes
namespace mazes
{
    /// @class grid_operations
    /// @brief Interface for grid navigation and manipulation operations
    class grid_operations
    {
    public:
        /// @brief Checks for nullity
        /// @param ops
        /// @return
        [[nodiscard]]
        static bool is_valid_ops(const grid_operations *ops) noexcept
        {
            return ops != nullptr;
        }

        /// @brief Destroys the grid_operations object and releases any associated resources.
        virtual ~grid_operations() = default;

        /// @brief Retrieves the dimensions as a tuple of three unsigned integers.
        /// @return A tuple containing three unsigned integers representing the dimensions.
        [[nodiscard]] virtual std::tuple<std::uint32_t, std::uint32_t, std::uint32_t> get_dimensions() const noexcept = 0;

        /// @brief Get neighbor by the cell's respective location
        /// @param c
        /// @param dir
        /// @return
        [[nodiscard]] virtual std::shared_ptr<cell> get_neighbor(std::shared_ptr<cell> const &c,
                                                                 Direction dir) const noexcept = 0;

        /// @brief Maximum number of neighbors a cell can have
        static constexpr std::size_t MAX_NEIGHBORS = 4;

        /// @brief Reusable neighbor buffer type for collect_neighbors
        using neighbor_buffer = std::array<std::shared_ptr<cell>, MAX_NEIGHBORS>;

        /// @brief Fill a caller-provided buffer with the neighbors of a cell, avoiding allocation
        /// @param c
        /// @param out Buffer to fill (reusable across calls)
        /// @return The number of neighbors written to out
        [[nodiscard]] virtual std::size_t collect_neighbors(std::shared_ptr<cell> const &c,
                                                        std::span<std::shared_ptr<cell>, MAX_NEIGHBORS> out) const noexcept = 0;

        /// @brief Get all the neighbors by the cell
        /// @deprecated Allocates on every call; use the buffer overload instead
        [[deprecated("use collect_neighbors(c, buffer), which does not allocate")]] [[nodiscard]]
        std::vector<std::shared_ptr<cell>> collect_neighbors(std::shared_ptr<cell> const &c) const noexcept
        {
            neighbor_buffer buf;
            const auto n = collect_neighbors(c, std::span<std::shared_ptr<cell>, MAX_NEIGHBORS>{buf});
            return {std::make_move_iterator(buf.begin()), std::make_move_iterator(buf.begin() + n)};
        }

        /// @brief Zero-copy view of all cells, in index order (level-major, then row, then column)
        /// @note The span is invalidated by resize()
        [[nodiscard]] virtual std::span<const std::shared_ptr<cell>> cells() const noexcept = 0;

        /// @brief Set neighbor for a cell in a given direction
        /// @param c
        /// @param dir
        /// @param neighbor
        /// @return
        virtual void set_neighbor(const std::shared_ptr<cell> &c, Direction dir,
                                  std::shared_ptr<cell> const &neighbor) noexcept = 0;

        // Convenience methods for accessing neighbors
        [[nodiscard]] virtual std::shared_ptr<cell> get_north(const std::shared_ptr<cell> &c) const noexcept = 0;
        [[nodiscard]] virtual std::shared_ptr<cell> get_south(const std::shared_ptr<cell> &c) const noexcept = 0;
        [[nodiscard]] virtual std::shared_ptr<cell> get_east(const std::shared_ptr<cell> &c) const noexcept = 0;
        [[nodiscard]] virtual std::shared_ptr<cell> get_west(const std::shared_ptr<cell> &c) const noexcept = 0;

        /// @brief Search for a cell by index
        /// @param index
        /// @return
        [[nodiscard]] virtual std::shared_ptr<cell> search(int index) const noexcept = 0;

        /// @brief Get the count of cells in the grid
        /// @return The number of cells in the grid
        [[nodiscard]] virtual std::size_t num_cells() const noexcept = 0;

        /// @brief Get the vertices
        /// @return A vector of vertices as tuples (x, y, z, w)
        [[nodiscard]] virtual std::vector<std::tuple<std::int32_t, std::int32_t, std::int32_t, std::int32_t>>
        get_vertices() const noexcept = 0;

        /// @brief Set the vertices
        /// @param vertices A vector of vertices as tuples (x, y, z, w)
        virtual void set_vertices(
            const std::vector<std::tuple<std::int32_t, std::int32_t, std::int32_t, std::int32_t>> &vertices) noexcept = 0;

        /// @brief Get the faces
        /// @return A vector of faces, where each face is a vector of vertex indices
        [[nodiscard]] virtual std::vector<std::vector<std::uint32_t>> get_faces() const noexcept = 0;

        /// @brief Set the faces
        /// @param faces A vector of faces, where each face is a vector of vertex indices
        virtual void set_faces(const std::vector<std::vector<std::uint32_t>> &faces) noexcept = 0;

        /// @brief Get the pixel data for image generation
        /// @return A vector of RGBA pixel data
        [[nodiscard]] virtual std::vector<std::uint8_t> get_pixels() const noexcept = 0;

        /// @brief Set the pixel data for image generation
        /// @param pixels A vector of RGBA pixel data
        virtual void set_pixels(const std::vector<std::uint8_t> &pixels) noexcept = 0;

        /// @brief Resize the grid to new dimensions, clearing all existing cells and topology.
        /// @param rows New row count  @param cols New column count  @param levels New level count
        virtual void resize(std::uint32_t rows, std::uint32_t cols, std::uint32_t levels) noexcept = 0;
    };
} // namespace mazes

#endif // GRID_OPERATIONS_H
