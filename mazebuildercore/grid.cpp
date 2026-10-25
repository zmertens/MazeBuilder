#include <MazeBuilder/grid.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>

#include <random>
#include <ranges>
#include <string>
#include <tuple>
#include <vector>

using namespace mazes;

/// @param rows 2u
/// @param columns 2u
/// @param levels 1u
grid::grid(std::uint8_t x, std::uint8_t y, std::uint8_t z)
    : grid(grid_operations::row_col_level{x, y, z})
{
}

/// @param dimens A tuple containing the dimensions of the grid (rows, columns, levels)
grid::grid(const grid_operations::row_col_level &dimens)
    : m_dimensions(dimens)
{
    m_pool.acquire(std::get<0>(dimens), std::get<1>(dimens), std::get<2>(dimens));
}

// Copy constructor: a copy owns its own cells
grid::grid(const grid &other)
    : grid(other.m_dimensions)
{
}

// Copy assignment operator
grid &grid::operator=(const grid &other)
{
    if (this != &other)
    {
        resize(other.m_dimensions);
    }
    return *this;
}

// Move constructor
grid::grid(grid &&other) noexcept
    : grid(other.m_dimensions)
{
}

// Move assignment operator
grid &grid::operator=(grid &&other) noexcept
{
    if (this != &other)
    {
        resize(other.m_dimensions);
    }
    return *this;
}

// Destructor
grid::~grid()
{
    // First clean up cell references
    grid::clear_cells();
}

void grid::clear_cells() noexcept
{
    m_topology.clear();
}

grid_operations::row_col_level grid::get_dimensions() const noexcept
{
    return this->m_dimensions;
}

std::shared_ptr<cell> grid::search(int index) const noexcept
{
    return m_pool.at(index);
}

int grid::index_of(const std::shared_ptr<cell> &c) const noexcept
{
    return c ? m_pool.index_of(c.get()) : -1;
}

std::size_t grid::num_cells() const noexcept
{
    return m_pool.size();
}

// Get the contents of a cell for this type of grid
std::string grid::contents_of([[maybe_unused]] std::shared_ptr<cell> const &c) const noexcept
{
    return " ";
}

// Get the background color for this type of grid
std::uint32_t grid::background_color_for([[maybe_unused]] std::shared_ptr<cell> const &c) const noexcept
{
    return 0xFFFFFFFF;
}

grid_operations &grid::operations() noexcept
{
    return *this;
}

const grid_operations &grid::operations() const noexcept
{
    return *this;
}

std::shared_ptr<cell> grid::get_neighbor(std::shared_ptr<cell> const &c, const Direction dir) const noexcept
{
    if (!c)
    {
        return nullptr;
    }

    // Calculate neighbor index on-demand instead of pre-computing topology
    auto [rows, columns, levels] = m_dimensions;
    const int current_index = index_of(c);
    if (current_index < 0)
    {
        return nullptr;
    }

    // Calculate 3D coordinates from index
    const int level = current_index / (rows * columns);
    const int remaining = current_index % (rows * columns);
    const int row = remaining / columns;
    const int col = remaining % columns;

    int neighbor_index = -1;

    switch (dir)
    {
    case Direction::NORTH:
        if (row > 0)
        {
            neighbor_index = level * (rows * columns) + (row - 1) * columns + col;
        }
        break;
    case Direction::SOUTH:
        if (row < static_cast<int>(rows) - 1)
        {
            neighbor_index = level * (rows * columns) + (row + 1) * columns + col;
        }
        break;
    case Direction::EAST:
        if (col < static_cast<int>(columns) - 1)
        {
            neighbor_index = level * (rows * columns) + row * columns + (col + 1);
        }
        break;
    case Direction::WEST:
        if (col > 0)
        {
            neighbor_index = level * (rows * columns) + row * columns + (col - 1);
        }
        break;
    }

    // Return the neighbor (will be created lazily if it doesn't exist)
    return (neighbor_index >= 0) ? search(neighbor_index) : nullptr;
}

std::size_t grid::collect_neighbors(std::shared_ptr<cell> const &c,
                                std::span<std::shared_ptr<cell>, MAX_NEIGHBORS> out) const noexcept
{
    if (!c)
    {
        return 0;
    }

    std::size_t count = 0;
    for (const auto dir : {Direction::NORTH, Direction::SOUTH, Direction::EAST, Direction::WEST})
    {
        if (auto n = get_neighbor(c, dir))
        {
            out[count++] = std::move(n);
        }
    }
    return count;
}

std::span<const std::shared_ptr<cell>> grid::cells() const noexcept
{
    return m_pool.view();
}

void grid::set_neighbor(const std::shared_ptr<cell> &c, Direction dir, std::shared_ptr<cell> const &neighbor) noexcept
{
    if (!c)
    {
        return;
    }

    if (neighbor)
    {
        m_topology[index_of(c)][dir] = index_of(neighbor);
    }
    else
    {
        // Remove the neighbor relationship
        auto cell_it = m_topology.find(index_of(c));
        if (cell_it != m_topology.end())
        {
            cell_it->second.erase(dir);
        }
    }
}

// Convenience methods for accessing neighbors
std::shared_ptr<cell> grid::get_north(const std::shared_ptr<cell> &c) const noexcept
{
    return get_neighbor(c, Direction::NORTH);
}

std::shared_ptr<cell> grid::get_south(const std::shared_ptr<cell> &c) const noexcept
{
    return get_neighbor(c, Direction::SOUTH);
}

std::shared_ptr<cell> grid::get_east(const std::shared_ptr<cell> &c) const noexcept
{
    return get_neighbor(c, Direction::EAST);
}

std::shared_ptr<cell> grid::get_west(const std::shared_ptr<cell> &c) const noexcept
{
    return get_neighbor(c, Direction::WEST);
}

/// @brief Get the vertices for wavefront object file generation
/// @return A vector of vertices as tuples (x, y, z, w)
std::vector<std::tuple<int, int, int, int>> grid::get_vertices() const noexcept
{
    return m_vertices;
}

/// @brief Set the vertices for wavefront object file generation
/// @param vertices A vector of vertices as tuples (x, y, z, w)
void grid::set_vertices(const std::vector<std::tuple<int, int, int, int>> &vertices) noexcept
{
    m_vertices = vertices;
}

/// @brief Get the faces for wavefront object file generation
/// @return A vector of faces, where each face is a vector of vertex indices
std::vector<std::vector<std::uint32_t>> grid::get_faces() const noexcept
{
    return m_faces;
}

/// @brief Set the faces for wavefront object file generation
/// @param faces A vector of faces, where each face is a vector of vertex indices
void grid::set_faces(const std::vector<std::vector<std::uint32_t>> &faces) noexcept
{
    m_faces = faces;
}

/// @brief Get the pixel data for image generation
/// @return A vector of RGBA pixel data
std::vector<std::uint8_t> grid::get_pixels() const noexcept
{
    return m_pixels;
}

/// @brief Set the pixel data for image generation
/// @param pixels A vector of RGBA pixel data
void grid::set_pixels(const std::vector<std::uint8_t> &pixels) noexcept
{
    m_pixels = pixels;
}

void grid::resize(grid_operations::row_col_level new_size) noexcept
{
    m_topology.clear();
    m_dimensions = new_size;
    m_pool.acquire(std::get<0>(new_size), std::get<1>(new_size), std::get<2>(new_size));
}
