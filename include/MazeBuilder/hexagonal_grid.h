#ifndef HEXAGONAL_GRID_H
#define HEXAGONAL_GRID_H

#include <MazeBuilder/grid.h>

#include <memory>
#include <tuple>
#include <vector>

namespace mazes
{

    class cell;

    /// @file hexagonal_grid.h
    /// @class hexagonal_grid
    /// @brief Hexagonal grid class for 2D maze generation with 6-way connectivity
    /// @details Uses pointy-top hexagonal topology with even-r offset coordinates.
    ///          Each interior cell has up to 6 neighbors: NORTHEAST, NORTHWEST,
    ///          EAST, WEST, SOUTHEAST, SOUTHWEST. There is no direct NORTH or SOUTH.
    class hexagonal_grid : public grid
    {

    public:
        /// @brief Construct a hexagonal grid using unsigned integers
        /// @param rows Number of rows
        /// @param columns Number of columns
        /// @param levels Number of levels
        explicit hexagonal_grid(unsigned int rows = 1u, unsigned int columns = 1u, unsigned int levels = 1u);

        /// @brief Construct a hexagonal grid using a tuple of unsigned integers
        /// @param dimens Tuple containing rows, columns, and levels
        explicit hexagonal_grid(std::tuple<unsigned int, unsigned int, unsigned int> dimens);

        /// @brief Copy constructor
        /// @param other The hexagonal_grid to copy from
        hexagonal_grid(const hexagonal_grid &other);

        /// @brief Copy assignment operator
        /// @param other The hexagonal_grid to copy from
        /// @return Reference to this hexagonal_grid
        hexagonal_grid &operator=(const hexagonal_grid &other);

        /// @brief Move constructor
        /// @param other The hexagonal_grid to move from
        hexagonal_grid(hexagonal_grid &&other) noexcept;

        /// @brief Move assignment operator
        /// @param other The hexagonal_grid to move from
        /// @return Reference to this hexagonal_grid
        hexagonal_grid &operator=(hexagonal_grid &&other) noexcept;

        /// @brief Destructor
        ~hexagonal_grid() override = default;

        /// @brief Get a neighbor cell in the given direction using hexagonal topology
        /// @param c The source cell
        /// @param dir The direction to look in
        /// @return Shared pointer to the neighbor cell, or nullptr if none exists
        std::shared_ptr<cell> get_neighbor(std::shared_ptr<cell> const &c, Direction dir) const noexcept override;

        /// @brief Get all 6 hex neighbors of a cell
        /// @param c The source cell
        /// @return Vector of up to 6 neighbor cells (only existing neighbors are included)
        std::vector<std::shared_ptr<cell>> get_neighbors(std::shared_ptr<cell> const &c) const noexcept override;

        /// @brief Returns nullptr: no direct NORTH neighbor in pointy-top hex topology
        /// @param c The source cell
        /// @return nullptr
        std::shared_ptr<cell> get_north(const std::shared_ptr<cell> &c) const noexcept override;

        /// @brief Returns nullptr: no direct SOUTH neighbor in pointy-top hex topology
        /// @param c The source cell
        /// @return nullptr
        std::shared_ptr<cell> get_south(const std::shared_ptr<cell> &c) const noexcept override;

        /// @brief Get the NORTHEAST neighbor using even-r offset coordinates
        /// @param c The source cell
        /// @return Shared pointer to the NORTHEAST neighbor, or nullptr if none exists
        std::shared_ptr<cell> get_northeast(const std::shared_ptr<cell> &c) const noexcept;

        /// @brief Get the NORTHWEST neighbor using even-r offset coordinates
        /// @param c The source cell
        /// @return Shared pointer to the NORTHWEST neighbor, or nullptr if none exists
        std::shared_ptr<cell> get_northwest(const std::shared_ptr<cell> &c) const noexcept;

        /// @brief Get the SOUTHEAST neighbor using even-r offset coordinates
        /// @param c The source cell
        /// @return Shared pointer to the SOUTHEAST neighbor, or nullptr if none exists
        std::shared_ptr<cell> get_southeast(const std::shared_ptr<cell> &c) const noexcept;

        /// @brief Get the SOUTHWEST neighbor using even-r offset coordinates
        /// @param c The source cell
        /// @return Shared pointer to the SOUTHWEST neighbor, or nullptr if none exists
        std::shared_ptr<cell> get_southwest(const std::shared_ptr<cell> &c) const noexcept;

    private:
        /// @brief Compute the neighbor index for a given direction in hex topology
        /// @param row Current row
        /// @param col Current column
        /// @param level Current level
        /// @param dir Direction to look in
        /// @param rows Total rows
        /// @param columns Total columns
        /// @return Neighbor index, or -1 if out of bounds
        int compute_hex_neighbor_index(int row, int col, int level, Direction dir,
                                       unsigned int rows, unsigned int columns) const noexcept;
    };

} // namespace mazes

#endif // HEXAGONAL_GRID_H
