#include <MazeBuilder/grid.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/lab.h>

#include <algorithm>
#include <functional>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

using namespace mazes;

/// @brief
/// @param rows
/// @param columns
/// @param height
grid::grid(unsigned int rows, unsigned int columns, unsigned int height)
    : grid::grid(std::make_tuple(rows, columns, height))
{

}

/// @brief
/// @param dimensions
grid::grid(std::tuple<unsigned int, unsigned int, unsigned int> dimens)
    : m_dimensions(dimens)
{
    auto total_cells = std::get<0>(m_dimensions) * std::get<1>(m_dimensions) * std::get<2>(m_dimensions);

    m_cells.reserve(total_cells);

    for (size_t i{0}; i < total_cells; ++i) {
        m_cells.emplace(static_cast<int32_t>(i), std::make_shared<cell>(static_cast<int32_t>(i)));
    }
}

// Copy constructor
grid::grid(const grid &other)
    : m_dimensions(other.m_dimensions)
{

}

// Copy assignment operator
grid &grid::operator=(const grid &other)
{
    if (this == &other)
    {
        return *this;
    }

    m_dimensions = other.m_dimensions;

    return *this;
}

// Move constructor
grid::grid(grid &&other) noexcept
    : m_dimensions(other.m_dimensions)
{

}

// Move assignment operator
grid &grid::operator=(grid &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    m_dimensions = other.m_dimensions;

    return *this;
}

// Destructor
grid::~grid()
{

    // First clean up cell references
    clear_cells();
}

void grid::clear_cells() noexcept
{

    // Clear topology first
    {
        std::lock_guard<std::mutex> lock(m_topology_mutex);
        m_topology.clear();
    }
}

std::tuple<unsigned int, unsigned int, unsigned int> grid::get_dimensions() const noexcept
{

    return this->m_dimensions;
}

std::shared_ptr<cell> grid::search(int index) const noexcept
{

    // First check if cell already exists
    if (auto cell_it = m_cells.find(index); cell_it != m_cells.cend())
    {
        return cell_it->second;
    }

    return nullptr;
}

int grid::num_cells() const noexcept
{

    return static_cast<int>(m_cells.size());
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

void grid::set_str(std::string const &str) noexcept
{
    this->m_str = str;
}

std::string grid::get_str() const noexcept
{
    return this->m_str;
}

std::shared_ptr<cell> grid::get_neighbor(std::shared_ptr<cell> const &c, Direction dir) const noexcept
{
    if (!c)
    {
        return nullptr;
    }

    // Calculate neighbor index on-demand instead of pre-computing topology
    auto [rows, columns, levels] = m_dimensions;
    int current_index = c->get_index();

    // Calculate 3D coordinates from index
    int level = current_index / (rows * columns);
    int remaining = current_index % (rows * columns);
    int row = remaining / columns;
    int col = remaining % columns;

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

std::vector<std::shared_ptr<cell>> grid::get_neighbors(std::shared_ptr<cell> const &c) const noexcept
{
    std::vector<std::shared_ptr<cell>> neighbors;

    if (!c)
    {
        return neighbors;
    }

    // Get neighbors in all four directions
    if (auto north = get_neighbor(c, Direction::NORTH))
    {
        neighbors.push_back(north);
    }
    if (auto south = get_neighbor(c, Direction::SOUTH))
    {
        neighbors.push_back(south);
    }
    if (auto east = get_neighbor(c, Direction::EAST))
    {
        neighbors.push_back(east);
    }
    if (auto west = get_neighbor(c, Direction::WEST))
    {
        neighbors.push_back(west);
    }

    return neighbors;
}

void grid::set_neighbor(const std::shared_ptr<cell> &c, Direction dir, std::shared_ptr<cell> const &neighbor) noexcept
{
    if (!c)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_topology_mutex);

    if (neighbor)
    {
        m_topology[c->get_index()][dir] = neighbor->get_index();
    }
    else
    {
        // Remove the neighbor relationship
        auto cell_it = m_topology.find(c->get_index());
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
