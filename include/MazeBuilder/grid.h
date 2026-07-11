#ifndef GRID_H
#define GRID_H

#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
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
        /// @param rows
        /// @param columns
        /// @param levels
        explicit grid(unsigned int rows = 1u, unsigned int columns = 1u, unsigned int levels = 1u);

        /// @brief Construct a grid using a tuple of unsigned integers
        /// @param dimens
        explicit grid(const std::tuple<unsigned int, unsigned int, unsigned int>& dimens);

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

        /// @brief
        /// @return
        grid_operations& operations() noexcept override;

        /// @brief
        /// @return
        const grid_operations& operations() const noexcept override;

        /// @brief Get the dimensions of the grid
        /// @return A tuple containing the number of rows, columns, and levels
        std::tuple<unsigned int, unsigned int, unsigned int> get_dimensions() const noexcept override;

        /// @brief Get detailed information of a cell in the grid
        /// @param c
        /// @return
        std::string contents_of(std::shared_ptr<cell> const& c) const noexcept override;

        /// @brief Get the background color for a cell in the grid
        /// @param c
        /// @return
        std::uint32_t background_color_for(std::shared_ptr<cell> const& c) const noexcept override;

        /// @brief Get neighbor by the cell's respective location
        /// @param c
        /// @param dir
        /// @return
        std::shared_ptr<cell> get_neighbor(std::shared_ptr<cell> const& c, direction dir) const noexcept override;

        /// @brief Get all the neighbors by the cell
        /// @param c
        /// @return
        std::vector<std::shared_ptr<cell>> get_neighbors(std::shared_ptr<cell> const& c) const noexcept override;

        /// @brief Set neighbor for a cell in a given direction
        /// @param c
        /// @param dir
        /// @param neighbor
        /// @return
        void set_neighbor(const std::shared_ptr<cell>& c, direction dir,
                          std::shared_ptr<cell> const& neighbor) noexcept override;

        // Convenience methods for accessing neighbors
        std::shared_ptr<cell> get_north(const std::shared_ptr<cell>& c) const noexcept override;
        std::shared_ptr<cell> get_south(const std::shared_ptr<cell>& c) const noexcept override;
        std::shared_ptr<cell> get_east(const std::shared_ptr<cell>& c) const noexcept override;
        std::shared_ptr<cell> get_west(const std::shared_ptr<cell>& c) const noexcept override;

        /// @brief Search for a cell by index
        /// @param index
        /// @return
        std::shared_ptr<cell> search(int index) const noexcept override;

        /// @brief Get the count of cells in the grid
        /// @return The number of cells in the grid
        int num_cells() const noexcept override;

        /// @brief Cleanup cells by cleaning up links within cells
        void clear_cells() noexcept override;

        /// @brief Set a string value
        /// @param str
        void set_str(std::string const& str) noexcept override;

        /// @brief Get a string value
        std::string get_str() const noexcept override;

        void set_file(std::string const& f) noexcept override;

        std::string get_file() const noexcept override;

        /// @brief Get the vertices
        /// @return A vector of vertices as tuples (x, y, z, w)
        std::vector<std::tuple<int, int, int, int>> get_vertices() const noexcept override;

        /// @brief Set the vertices
        /// @param vertices A vector of vertices as tuples (x, y, z, w)
        void set_vertices(const std::vector<std::tuple<int, int, int, int>>& vertices) noexcept override;

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
        void resize(unsigned int rows, unsigned int cols, unsigned int levels) noexcept override;

    private:
        std::unordered_map<int, std::shared_ptr<cell>> m_cells;

        std::tuple<unsigned int, unsigned int, unsigned int> m_dimensions;

        // Store topology - which cell is neighbor to which in what direction
        // Key: cell index, Value: map of direction to neighbor cell index
        mutable std::mutex m_topology_mutex;
        std::unordered_map<int, std::unordered_map<direction, int>> m_topology;

        // Arbitrary data
        std::string m_file;
        std::string m_str;

        // 3D data
        std::vector<std::tuple<int, int, int, int>> m_vertices;
        std::vector<std::vector<std::uint32_t>> m_faces;

        // Image data
        std::vector<std::uint8_t> m_pixels;
    };
} // namespace mazes

#endif // GRID_H
