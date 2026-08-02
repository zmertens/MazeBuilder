#include <MazeBuilder/grid.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>

#include <random>
#include <string>
#include <tuple>
#include <vector>

using namespace mazes;

/// @brief
/// @param rows
/// @param columns
/// @param levels
grid::grid(unsigned int rows, unsigned int columns, unsigned int levels)
    : grid(std::make_tuple(rows, columns, levels))
{
}

/// @brief
/// @param dimens A tuple containing the dimensions of the grid (rows, columns, levels)
grid::grid(const std::tuple<unsigned int, unsigned int, unsigned int>& dimens)
    : m_dimensions(dimens)
{
    const auto total_cells = std::get<0>(m_dimensions) * std::get<1>(m_dimensions) * std::get<2>(m_dimensions);

    m_cells.reserve(total_cells);

    for (std::size_t i{0}; i < total_cells; ++i)
    {
        m_cells.emplace(static_cast<int32_t>(i), std::make_shared<cell>(static_cast<int32_t>(i)));
    }
}

// Copy constructor
grid::grid(const grid& other)
    : m_cells(other.m_cells), m_dimensions(other.m_dimensions)
{
}

// Copy assignment operator
grid& grid::operator=(const grid& other)
{
    if (this == &other)
    {
        return *this;
    }

    m_dimensions = other.m_dimensions;

    m_cells = other.m_cells;

    return *this;
}

// Move constructor
grid::grid(grid&& other) noexcept
    : grid(other.m_dimensions)
{
}

// Move assignment operator
grid& grid::operator=(grid&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    m_dimensions = other.m_dimensions;

    m_cells = other.m_cells;

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
    // Clear topology first
    {
        std::lock_guard lock(m_topology_mutex);
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
    if (const auto cell_it = m_cells.find(index); cell_it != m_cells.cend())
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
std::string grid::contents_of([[maybe_unused]] std::shared_ptr<cell> const& c) const noexcept
{
    return " ";
}

// Get the background color for this type of grid
std::uint32_t grid::background_color_for([[maybe_unused]] std::shared_ptr<cell> const& c) const noexcept
{
    return 0xFFFFFFFF;
}

grid_operations& grid::operations() noexcept
{
    return *this;
}

const grid_operations& grid::operations() const noexcept
{
    return *this;
}

void grid::set_str(std::string const& str) noexcept
{
    this->m_str = str;
}

std::string grid::get_str() const noexcept
{
    return this->m_str;
}

void grid::set_file(std::string const& f) noexcept
{
    this->m_file = f;
}

std::string grid::get_file() const noexcept
{
    return this->m_file;
}

std::shared_ptr<cell> grid::get_neighbor(std::shared_ptr<cell> const& c, const direction dir) const noexcept
{
    if (!c)
    {
        return nullptr;
    }

    // Calculate neighbor index on-demand instead of pre-computing topology
    auto [rows, columns, levels] = m_dimensions;
    const int current_index = c->get_index();

    // Calculate 3D coordinates from index
    const int level = current_index / (rows * columns);
    const int remaining = current_index % (rows * columns);
    const int row = remaining / columns;
    const int col = remaining % columns;

    int neighbor_index = -1;

    switch (dir)
    {
    case direction::NORTH:
        if (row > 0)
        {
            neighbor_index = level * (rows * columns) + (row - 1) * columns + col;
        }
        break;
    case direction::SOUTH:
        if (row < static_cast<int>(rows) - 1)
        {
            neighbor_index = level * (rows * columns) + (row + 1) * columns + col;
        }
        break;
    case direction::EAST:
        if (col < static_cast<int>(columns) - 1)
        {
            neighbor_index = level * (rows * columns) + row * columns + (col + 1);
        }
        break;
    case direction::WEST:
        if (col > 0)
        {
            neighbor_index = level * (rows * columns) + row * columns + (col - 1);
        }
        break;
    }

    // Return the neighbor (will be created lazily if it doesn't exist)
    return (neighbor_index >= 0) ? search(neighbor_index) : nullptr;
}

std::vector<std::shared_ptr<cell>> grid::get_neighbors(std::shared_ptr<cell> const& c) const noexcept
{
    std::vector<std::shared_ptr<cell>> neighbors;

    if (!c)
    {
        return neighbors;
    }

    // Get neighbors in all four directions
    if (const auto north = get_neighbor(c, direction::NORTH))
    {
        neighbors.push_back(north);
    }
    if (const auto south = get_neighbor(c, direction::SOUTH))
    {
        neighbors.push_back(south);
    }
    if (const auto east = get_neighbor(c, direction::EAST))
    {
        neighbors.push_back(east);
    }
    if (const auto west = get_neighbor(c, direction::WEST))
    {
        neighbors.push_back(west);
    }

    return neighbors;
}

void grid::set_neighbor(const std::shared_ptr<cell>& c, direction dir, std::shared_ptr<cell> const& neighbor) noexcept
{
    if (!c)
    {
        return;
    }

    std::lock_guard lock(m_topology_mutex);

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
std::shared_ptr<cell> grid::get_north(const std::shared_ptr<cell>& c) const noexcept
{
    return get_neighbor(c, direction::NORTH);
}

std::shared_ptr<cell> grid::get_south(const std::shared_ptr<cell>& c) const noexcept
{
    return get_neighbor(c, direction::SOUTH);
}

std::shared_ptr<cell> grid::get_east(const std::shared_ptr<cell>& c) const noexcept
{
    return get_neighbor(c, direction::EAST);
}

std::shared_ptr<cell> grid::get_west(const std::shared_ptr<cell>& c) const noexcept
{
    return get_neighbor(c, direction::WEST);
}

/// @brief Get the vertices for wavefront object file generation
/// @return A vector of vertices as tuples (x, y, z, w)
std::vector<std::tuple<int, int, int, int>> grid::get_vertices() const noexcept
{
    return m_vertices;
}

/// @brief Set the vertices for wavefront object file generation
/// @param vertices A vector of vertices as tuples (x, y, z, w)
void grid::set_vertices(const std::vector<std::tuple<int, int, int, int>>& vertices) noexcept
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
void grid::set_faces(const std::vector<std::vector<std::uint32_t>>& faces) noexcept
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
void grid::set_pixels(const std::vector<std::uint8_t>& pixels) noexcept
{
    m_pixels = pixels;
}

void grid::resize(unsigned int rows, unsigned int cols, unsigned int levels) noexcept
{
    // Clear topology
    {
        std::lock_guard lock(m_topology_mutex);
        m_topology.clear();
    }

    // Replace all cells
    m_cells.clear();
    m_dimensions = std::make_tuple(rows, cols, levels);

    const auto total = static_cast<size_t>(rows) * cols * levels;
    m_cells.reserve(total);
    for (size_t i = 0; i < total; ++i)
    {
        m_cells.emplace(static_cast<int32_t>(i),
                        std::make_shared<cell>(static_cast<int32_t>(i)));
    }
}

