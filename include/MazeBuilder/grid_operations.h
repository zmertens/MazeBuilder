#ifndef GRID_OPERATIONS_H
#define GRID_OPERATIONS_H

#include <MazeBuilder/cell.h>
#include <MazeBuilder/enums.h>

#include <memory>
#include <tuple>
#include <vector>

namespace mazes
{

    /// @file grid_operations.h
    /// @class grid_operations
    /// @brief Interface for grid navigation and manipulation operations
    class grid_operations
    {

    public:
        /// @brief Destroys the grid_operations object and releases any associated resources.
        virtual ~grid_operations() = default;

        /// @brief Retrieves the dimensions as a tuple of three unsigned integers.
        /// @return A tuple containing three unsigned integers representing the dimensions.
        virtual std::tuple<unsigned int, unsigned int, unsigned int> get_dimensions() const noexcept = 0;

        /// @brief Get neighbor by the cell's respective location
        /// @param c
        /// @param dir
        /// @return
        virtual std::shared_ptr<cell> get_neighbor(std::shared_ptr<cell> const &c, Direction dir) const noexcept = 0;

        /// @brief Get all the neighbors by the cell
        /// @param c
        /// @return
        virtual std::vector<std::shared_ptr<cell>> get_neighbors(std::shared_ptr<cell> const &c) const noexcept = 0;

        /// @brief Set neighbor for a cell in a given direction
        /// @param c
        /// @param dir
        /// @param neighbor
        /// @return
        virtual void set_neighbor(const std::shared_ptr<cell> &c, Direction dir, std::shared_ptr<cell> const &neighbor) noexcept = 0;

        // Convenience methods for accessing neighbors
        virtual std::shared_ptr<cell> get_north(const std::shared_ptr<cell> &c) const noexcept = 0;
        virtual std::shared_ptr<cell> get_south(const std::shared_ptr<cell> &c) const noexcept = 0;
        virtual std::shared_ptr<cell> get_east(const std::shared_ptr<cell> &c) const noexcept = 0;
        virtual std::shared_ptr<cell> get_west(const std::shared_ptr<cell> &c) const noexcept = 0;

        /// @brief Search for a cell by index
        /// @param index
        /// @return
        virtual std::shared_ptr<cell> search(int index) const noexcept = 0;

        /// @brief Get the count of cells in the grid
        /// @return The number of cells in the grid
        virtual int num_cells() const noexcept = 0;

        /// @brief Cleanup cells by cleaning up links within cells
        virtual void clear_cells() noexcept = 0;

        virtual void set_str(std::string const &str) noexcept = 0;

        virtual std::string get_str() const noexcept = 0;

        /// @brief Get the vertices for wavefront object file generation
        /// @return A vector of vertices as tuples (x, y, z, w)
        virtual std::vector<std::tuple<int, int, int, int>> get_vertices() const noexcept = 0;

        /// @brief Set the vertices for wavefront object file generation
        /// @param vertices A vector of vertices as tuples (x, y, z, w)
        virtual void set_vertices(const std::vector<std::tuple<int, int, int, int>> &vertices) noexcept = 0;

        /// @brief Get the faces for wavefront object file generation
        /// @return A vector of faces, where each face is a vector of vertex indices
        virtual std::vector<std::vector<std::uint32_t>> get_faces() const noexcept = 0;

        /// @brief Set the faces for wavefront object file generation
        /// @param faces A vector of faces, where each face is a vector of vertex indices
        virtual void set_faces(const std::vector<std::vector<std::uint32_t>> &faces) noexcept = 0;
    };

} // namespace mazes

#endif // GRID_OPERATIONS_H
