#ifndef GRID_H
#define GRID_H

#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/maze_adapter.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace mazes
{

    class cell;

    /// @file grid.h
    /// @class grid
    /// @brief General purpose grid class for 2D maze generation
    class grid : public grid_interface, public grid_operations
    {

    public:
        /// @brief
        /// @param r
        /// @param c
        /// @param l
        explicit grid(unsigned int r = 1u, unsigned int c = 1u, unsigned int l = 1u);

        /// @brief
        /// @param dimens
        explicit grid(std::tuple<unsigned int, unsigned int, unsigned int> dimens);

        /// @brief Copy constructor
        /// @param other
        grid(const grid &other);

        /// @brief Assignment operator
        /// @param other
        /// @return
        grid &operator=(const grid &other);

        /// @brief Move constructor
        /// @param other
        grid(grid &&other) noexcept;

        /// @brief Move assignment operator
        /// @param other
        /// @return
        grid &operator=(grid &&other) noexcept;

        /// @brief Destructor
        ~grid() override;

        /// @brief Get detailed information of a cell in the grid
        /// @param c
        /// @return
        virtual std::string contents_of(std::shared_ptr<cell> const &c) const noexcept override;

        /// @brief Get the background color for a cell in the grid
        /// @param c
        /// @return
        virtual std::uint32_t background_color_for(std::shared_ptr<cell> const &c) const noexcept override;

        /// @brief
        /// @return
        grid_operations &operations() noexcept override;

        /// @brief
        /// @return
        const grid_operations &operations() const noexcept override;

        std::tuple<unsigned int, unsigned int, unsigned int> get_dimensions() const noexcept override;

        /// @brief Get neighbor by the cell's respective location
        /// @param c
        /// @param dir
        /// @return
        virtual std::shared_ptr<cell> get_neighbor(std::shared_ptr<cell> const &c, Direction dir) const noexcept override;

        /// @brief Get all the neighbors by the cell
        /// @param c
        /// @return
        virtual std::vector<std::shared_ptr<cell>> get_neighbors(std::shared_ptr<cell> const &c) const noexcept override;

        /// @brief Set neighbor for a cell in a given direction
        /// @param c
        /// @param dir
        /// @param neighbor
        /// @return
        virtual void set_neighbor(const std::shared_ptr<cell> &c, Direction dir, std::shared_ptr<cell> const &neighbor) noexcept override;

        /// @brief Transformation and display cells
        /// @param cells Vector to fill with cells
        void sort(std::vector<std::shared_ptr<cell>> &cells) const noexcept override;

        // Convenience methods for accessing neighbors
        virtual std::shared_ptr<cell> get_north(const std::shared_ptr<cell> &c) const noexcept override;
        virtual std::shared_ptr<cell> get_south(const std::shared_ptr<cell> &c) const noexcept override;
        virtual std::shared_ptr<cell> get_east(const std::shared_ptr<cell> &c) const noexcept override;
        virtual std::shared_ptr<cell> get_west(const std::shared_ptr<cell> &c) const noexcept override;

        /// @brief Search for a cell by index
        /// @param index
        /// @return
        virtual std::shared_ptr<cell> search(int index) const noexcept override;

        virtual std::vector<std::shared_ptr<cell>> get_cells() const noexcept override;

        /// @brief Get the count of cells in the grid
        /// @return The number of cells in the grid
        virtual int num_cells() const noexcept override;

        /// @brief Cleanup cells by cleaning up links within cells
        virtual void clear_cells() noexcept override;

        /// @brief Set cells and build topology from them
        /// @param cells Vector of pre-configured cells
        /// @return true if successful, false otherwise
        virtual bool set_cells(const std::vector<std::shared_ptr<cell>> &cells) noexcept override;

        virtual void set_str(std::string const &str) noexcept override;

        virtual std::string get_str() const noexcept override;

        /// @brief Get the vertices for wavefront object file generation
        /// @return A vector of vertices as tuples (x, y, z, w)
        virtual std::vector<std::tuple<int, int, int, int>> get_vertices() const noexcept override;

        /// @brief Set the vertices for wavefront object file generation
        /// @param vertices A vector of vertices as tuples (x, y, z, w)
        virtual void set_vertices(const std::vector<std::tuple<int, int, int, int>> &vertices) noexcept override;

        /// @brief Get the faces for wavefront object file generation
        /// @return A vector of faces, where each face is a vector of vertex indices
        virtual std::vector<std::vector<std::uint32_t>> get_faces() const noexcept override;

        /// @brief Set the faces for wavefront object file generation
        /// @param faces A vector of faces, where each face is a vector of vertex indices
        virtual void set_faces(const std::vector<std::vector<std::uint32_t>> &faces) noexcept override;

        /// @brief Get the maze adapter for advanced cell operations
        /// @return Const reference to the maze adapter
        const maze_adapter &get_maze_adapter() const noexcept;

        /// @brief Get a mutable maze adapter for advanced cell operations
        /// @return Reference to the maze adapter
        maze_adapter &get_maze_adapter() noexcept;

    private:
        /// @brief Calculate the flat index for a 2D grid
        std::function<int(unsigned int, unsigned int)> m_calculate_cell_index;

        std::unordered_map<int, std::shared_ptr<cell>> m_cells;

        /// @brief Maze adapter for efficient cell operations
        mutable maze_adapter m_maze_adapter;
        mutable std::mutex m_adapter_mutex;

        std::tuple<unsigned int, unsigned int, unsigned int> m_dimensions;

        // Store topology - which cell is neighbor to which in what direction
        // Key: cell index, Value: map of direction to neighbor cell index
        mutable std::mutex m_topology_mutex;
        std::unordered_map<int, std::unordered_map<Direction, int>> m_topology;

        std::atomic<bool> m_configured;

        std::string m_str;

        // Wavefront object data
        std::vector<std::tuple<int, int, int, int>> m_vertices;
        std::vector<std::vector<std::uint32_t>> m_faces;

        /// @brief Update the maze adapter with current cell data
        void update_maze_adapter() const;
    };

} // namespace mazes

#endif // GRID_H
