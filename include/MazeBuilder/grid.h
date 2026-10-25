#ifndef GRID_H
#define GRID_H

#include <MazeBuilder/cell_pool.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

/// @file grid.h
/// @namespace mazes
namespace mazes
{
    class cell;

    /// @class grid
    /// @brief General purpose grid class for 2D maze generation
    class grid : public grid_interface, public grid_operations
    {
    public:
        /// @brief Construct a grid using unsigned integers
        /// @param x The number of rows
        /// @param y The number of columns
        /// @param z The number of levels
        explicit grid(std::uint8_t x = 2u, std::uint8_t y = 2u, std::uint8_t z = 1u);

        /// @brief Construct a grid using a tuple of unsigned integers
        /// @param dimensions A tuple containing the new row, column, and level counts
        explicit grid(const grid_operations::row_col_level& dimensions);

        /// @brief Copy constructor
        /// @param other
        grid(const grid& other);

        /// @brief Assignment operator
        /// @param other
        /// @return
        grid& operator=(const grid& other);

        /// @brief Move constructor
        /// @param other
        grid(grid&& other) noexcept;

        /// @brief Move assignment operator
        /// @param other
        /// @return
        grid& operator=(grid&& other) noexcept;

        /// @brief Destructor
        ~grid() override;

        /// @brief Get access to grid operations interface
        /// @return A reference to the grid operations interface
        grid_operations& operations() noexcept override;

        /// @brief Get access to const grid operations interface
        /// @return A const reference to the grid operations interface
        const grid_operations& operations() const noexcept override;

        /// @brief Get the dimensions of the grid
        /// @return A tuple containing the number of rows, columns, and levels
        grid_operations::row_col_level get_dimensions() const noexcept override;

        /// @brief Get detailed information of a cell in the grid
        /// @param c A shared pointer to the cell for which to retrieve information.
        /// @return A string containing detailed information about the specified cell.
        std::string contents_of(std::shared_ptr<cell> const& c) const noexcept override;

        /// @brief Get the background color for a cell in the grid
        /// @param c A shared pointer to the cell for which to determine the background color.
        /// @return An 8-bit unsigned integer representing the background color of the cell.
        std::uint8_t background_color_for(std::shared_ptr<cell> const& c) const noexcept override;

        /// @brief Get neighbor by the cell's respective location
        /// @param c A shared pointer to the cell whose neighbor is to be retrieved.
        /// @param dir The direction in which to look for the neighbor.
        /// @return A shared pointer to the neighboring cell, or nullptr if no neighbor exists.
        std::shared_ptr<cell> get_neighbor(std::shared_ptr<cell> const& c, Direction dir) const noexcept override;

        std::size_t collect_neighbors(std::shared_ptr<cell> const& c,
                                  std::span<std::shared_ptr<cell>, MAX_NEIGHBORS> out) const noexcept override;

        std::span<const std::shared_ptr<cell>> cells() const noexcept override;

        /// @brief Set neighbor for a cell in a given direction
        /// @param c A shared pointer to the cell for which to set the neighbor.
        /// @param dir The direction in which to set the neighbor.
        /// @param neighbor A shared pointer to the neighboring cell to be set.
        /// @return None
        void set_neighbor(const std::shared_ptr<cell>& c, Direction dir,
            std::shared_ptr<cell> const& neighbor) noexcept override;

        // Convenience methods for accessing neighbors
        std::shared_ptr<cell> get_north(const std::shared_ptr<cell>& c) const noexcept override;
        std::shared_ptr<cell> get_south(const std::shared_ptr<cell>& c) const noexcept override;
        std::shared_ptr<cell> get_east(const std::shared_ptr<cell>& c) const noexcept override;
        std::shared_ptr<cell> get_west(const std::shared_ptr<cell>& c) const noexcept override;

        /// @brief Search for a cell by index
        /// @param index The linear index of the cell to search for.
        /// @return A shared pointer to the cell if found, or nullptr if not found.
        std::shared_ptr<cell> search(int index) const noexcept override;

        /// @brief Linear index of a cell
        /// @param c A shared pointer to the cell whose index is to be determined.
        /// @return The index, or -1 if the cell does not belong to this grid
        int index_of(const std::shared_ptr<cell>& c) const noexcept override;

        /// @brief Get the count of cells in the grid
        /// @return The number of cells in the grid
        std::size_t num_cells() const noexcept override;

        /// @brief Cleanup cells by cleaning up links within cells
        void clear_cells() noexcept override;

        using row_col_levels_w = std::tuple<grid_operations::row_col_level, std::uint8_t>;

        /// @brief Get the vertices
        /// @return A vector of vertices as tuples (x, y, z, w)
        std::vector<row_col_levels_w> get_vertices() const noexcept override;

        /// @brief Set the vertices
        /// @param vertices A vector of vertices as tuples (row, col, level, w)
        void set_vertices(const std::vector<row_col_levels_w>& vertices) noexcept override;

        /// @brief Get the faces
        /// @return A vector of faces, where each face is a vector of vertex indices
        std::vector<std::vector<std::uint32_t>> get_faces() const noexcept override;

        /// @brief Set the faces
        /// @param faces A vector of faces, where each face is a vector of vertex indices
        void set_faces(const std::vector<std::vector<std::uint32_t>>& faces) noexcept override;

        /// @brief Get the pixel data for image generation
        /// @return A vector of RGBA pixel data
        std::vector<std::uint8_t> get_pixels() const noexcept override;

        /// @brief Set the pixel data for image generation
        /// @param pixels A vector of RGBA pixel data
        void set_pixels(const std::vector<std::uint8_t>& pixels) noexcept override;

        /// @brief Resize: clear all cells/topology and reset dimensions.
        void resize(grid_operations::row_col_level new_size) noexcept override;

    private:
        // Reusable cells, grown lazily and recycled by resize()
        cell_pool m_pool;

        grid_operations::row_col_level m_dimensions;

        // Store topology - which cell is neighbor to which in what direction
        // Key: cell index, Value: map of direction to neighbor cell index
        std::unordered_map<std::uint8_t, std::unordered_map<Direction, std::uint8_t>> m_topology;

        // 3D data
        std::vector<row_col_levels_w> m_vertices;
        std::vector<std::vector<std::uint32_t>> m_faces;

        // Image data
        std::vector<std::uint8_t> m_pixels;
    };
} // namespace mazes

#endif // GRID_H
