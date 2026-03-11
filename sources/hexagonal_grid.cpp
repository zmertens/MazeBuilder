#include <MazeBuilder/hexagonal_grid.h>

#include <MazeBuilder/cell.h>

#include <tuple>
#include <vector>

using namespace mazes;

/// @brief Construct a hexagonal grid using unsigned integers
hexagonal_grid::hexagonal_grid(unsigned int rows, unsigned int columns, unsigned int levels)
    : grid(rows, columns, levels)
{
}

/// @brief Construct a hexagonal grid using a tuple of unsigned integers
hexagonal_grid::hexagonal_grid(std::tuple<unsigned int, unsigned int, unsigned int> dimens)
    : grid(dimens)
{
}

// Copy constructor
hexagonal_grid::hexagonal_grid(const hexagonal_grid &other)
    : grid(other)
{
}

// Copy assignment operator
hexagonal_grid &hexagonal_grid::operator=(const hexagonal_grid &other)
{
    if (this == &other)
    {
        return *this;
    }

    grid::operator=(other);

    return *this;
}

// Move constructor
hexagonal_grid::hexagonal_grid(hexagonal_grid &&other) noexcept
    : grid(std::move(other))
{
}

// Move assignment operator
hexagonal_grid &hexagonal_grid::operator=(hexagonal_grid &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    grid::operator=(std::move(other));

    return *this;
}

/// @brief Compute the neighbor index for a given direction in hex (pointy-top, even-r offset)
/// @details Even-r offset: even rows are not shifted; odd rows are shifted right.
///          For even rows (row % 2 == 0):
///            NE: (row-1, col),   NW: (row-1, col-1)
///            SE: (row+1, col),   SW: (row+1, col-1)
///            E:  (row,   col+1), W:  (row,   col-1)
///          For odd rows (row % 2 == 1):
///            NE: (row-1, col+1), NW: (row-1, col)
///            SE: (row+1, col+1), SW: (row+1, col)
///            E:  (row,   col+1), W:  (row,   col-1)
int hexagonal_grid::compute_hex_neighbor_index(int row, int col, int level, Direction dir,
                                                unsigned int rows, unsigned int columns) const noexcept
{
    const bool even_row = (row % 2 == 0);

    int neighbor_row = -1;
    int neighbor_col = -1;

    switch (dir)
    {
    case Direction::NORTHEAST:
        neighbor_row = row - 1;
        neighbor_col = even_row ? col : col + 1;
        break;
    case Direction::NORTHWEST:
        neighbor_row = row - 1;
        neighbor_col = even_row ? col - 1 : col;
        break;
    case Direction::SOUTHEAST:
        neighbor_row = row + 1;
        neighbor_col = even_row ? col : col + 1;
        break;
    case Direction::SOUTHWEST:
        neighbor_row = row + 1;
        neighbor_col = even_row ? col - 1 : col;
        break;
    case Direction::EAST:
        neighbor_row = row;
        neighbor_col = col + 1;
        break;
    case Direction::WEST:
        neighbor_row = row;
        neighbor_col = col - 1;
        break;
    default:
        return -1;
    }

    // Bounds check
    if (neighbor_row < 0 || neighbor_row >= static_cast<int>(rows) ||
        neighbor_col < 0 || neighbor_col >= static_cast<int>(columns))
    {
        return -1;
    }

    return level * static_cast<int>(rows * columns) + neighbor_row * static_cast<int>(columns) + neighbor_col;
}

std::shared_ptr<cell> hexagonal_grid::get_neighbor(std::shared_ptr<cell> const &c, Direction dir) const noexcept
{
    if (!c)
    {
        return nullptr;
    }

    auto [rows, columns, levels] = get_dimensions();

    const int current_index = c->get_index();
    const int level = current_index / static_cast<int>(rows * columns);
    const int remaining = current_index % static_cast<int>(rows * columns);
    const int row = remaining / static_cast<int>(columns);
    const int col = remaining % static_cast<int>(columns);

    const int neighbor_index = compute_hex_neighbor_index(row, col, level, dir, rows, columns);

    return (neighbor_index >= 0) ? search(neighbor_index) : nullptr;
}

std::vector<std::shared_ptr<cell>> hexagonal_grid::get_neighbors(std::shared_ptr<cell> const &c) const noexcept
{
    std::vector<std::shared_ptr<cell>> neighbors;

    if (!c)
    {
        return neighbors;
    }

    // Hexagonal grid has up to 6 neighbors: NE, NW, E, W, SE, SW
    if (auto ne = get_neighbor(c, Direction::NORTHEAST))
    {
        neighbors.push_back(ne);
    }
    if (auto nw = get_neighbor(c, Direction::NORTHWEST))
    {
        neighbors.push_back(nw);
    }
    if (auto e = get_neighbor(c, Direction::EAST))
    {
        neighbors.push_back(e);
    }
    if (auto w = get_neighbor(c, Direction::WEST))
    {
        neighbors.push_back(w);
    }
    if (auto se = get_neighbor(c, Direction::SOUTHEAST))
    {
        neighbors.push_back(se);
    }
    if (auto sw = get_neighbor(c, Direction::SOUTHWEST))
    {
        neighbors.push_back(sw);
    }

    return neighbors;
}

/// @brief Returns nullptr: no direct NORTH neighbor in pointy-top hex topology
std::shared_ptr<cell> hexagonal_grid::get_north(const std::shared_ptr<cell> &c) const noexcept
{
    (void)c;
    return nullptr;
}

/// @brief Returns nullptr: no direct SOUTH neighbor in pointy-top hex topology
std::shared_ptr<cell> hexagonal_grid::get_south(const std::shared_ptr<cell> &c) const noexcept
{
    (void)c;
    return nullptr;
}

std::shared_ptr<cell> hexagonal_grid::get_northeast(const std::shared_ptr<cell> &c) const noexcept
{
    return get_neighbor(c, Direction::NORTHEAST);
}

std::shared_ptr<cell> hexagonal_grid::get_northwest(const std::shared_ptr<cell> &c) const noexcept
{
    return get_neighbor(c, Direction::NORTHWEST);
}

std::shared_ptr<cell> hexagonal_grid::get_southeast(const std::shared_ptr<cell> &c) const noexcept
{
    return get_neighbor(c, Direction::SOUTHEAST);
}

std::shared_ptr<cell> hexagonal_grid::get_southwest(const std::shared_ptr<cell> &c) const noexcept
{
    return get_neighbor(c, Direction::SOUTHWEST);
}
