#include <MazeBuilder/grid.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/maze_adapter.h>

#include <algorithm>
#include <functional>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#if defined(MAZE_DEBUG)
#include <iostream>
#endif

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
    : m_dimensions(dimens), m_calculate_cell_index{[this](auto row, auto col) -> int
                                                   { return row * std::get<1>(this->m_dimensions) + col; }},
      m_configured{false}
{
}

// Copy constructor
grid::grid(const grid &other)
    : m_dimensions(other.m_dimensions)
{
    // Copy maze adapter if needed
    std::lock_guard<std::mutex> lock(other.m_adapter_mutex);
    m_maze_adapter = other.m_maze_adapter;
}

// Copy assignment operator
grid &grid::operator=(const grid &other)
{
    if (this == &other)
    {
        return *this;
    }
    m_dimensions = other.m_dimensions;

    // Copy maze adapter
    std::lock_guard<std::mutex> lock1(m_adapter_mutex);
    std::lock_guard<std::mutex> lock2(other.m_adapter_mutex);
    m_maze_adapter = other.m_maze_adapter;

    return *this;
}

// Move constructor
grid::grid(grid &&other) noexcept
    : m_dimensions(other.m_dimensions)
{
    // Move maze adapter
    std::lock_guard<std::mutex> lock(other.m_adapter_mutex);
    m_maze_adapter = std::move(other.m_maze_adapter);
}

// Move assignment operator
grid &grid::operator=(grid &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }
    m_dimensions = other.m_dimensions;

    // Move maze adapter
    std::lock_guard<std::mutex> lock1(m_adapter_mutex);
    std::lock_guard<std::mutex> lock2(other.m_adapter_mutex);
    m_maze_adapter = std::move(other.m_maze_adapter);

    return *this;
}

// Destructor
grid::~grid()
{
    // First clean up cell references
    clear_cells();

    m_configured = false;
    m_dimensions = {0, 0, 0};
    m_calculate_cell_index = {};
}

void grid::clear_cells() noexcept
{
    // First, clear topology information
    {
        std::lock_guard<std::mutex> lock(m_topology_mutex);
        m_topology.clear();
    }

    // Break all links between cells using maze_adapter
    {
        std::lock_guard<std::mutex> lock(m_adapter_mutex);
        if (!m_maze_adapter.empty())
        {
            for (const auto &cell_ptr : m_maze_adapter)
            {
                if (cell_ptr)
                {
                    // Unlink all links - this should remove all connections between cells
                    auto links = cell_ptr->get_links();
                    for (auto &[linked_cell, _] : links)
                    {
                        if (linked_cell)
                        {
                            // Use remove_link which doesn't try to modify the other cell's links
                            cell_ptr->remove_link(linked_cell);
                        }
                    }

                    // Extra cleanup
                    cell_ptr->cleanup_links();
                }
            }
        }

        // Clear the maze adapter
        m_maze_adapter = maze_adapter{};
    }

    // After breaking connections, clear the container
    m_cells.clear();

    // Complete reset - swap with an empty map to ensure memory is released
    std::unordered_map<int, std::shared_ptr<cell>>().swap(m_cells);
}

std::tuple<unsigned int, unsigned int, unsigned int> grid::get_dimensions() const noexcept
{
    return this->m_dimensions;
}

std::shared_ptr<cell> grid::search(int index) const noexcept
{
    // First check if cell already exists
    auto cell_it = m_cells.find(index);
    if (cell_it != m_cells.end())
    {
        return cell_it->second;
    }

    // Lazy creation: create cell on-demand if it's within valid bounds
    auto [rows, columns, levels] = m_dimensions;
    int max_index = static_cast<int>(rows * columns * levels);

    if (index >= 0 && index < max_index)
    {
        // Create the cell on-demand
        auto new_cell = std::make_shared<cell>(index);

        // Store it in the map (const_cast is safe here for lazy initialization)
        const_cast<grid *>(this)->m_cells[index] = new_cell;

        // Update maze adapter lazily
        std::lock_guard<std::mutex> lock(m_adapter_mutex);
        const_cast<grid *>(this)->update_maze_adapter();

        return new_cell;
    }

    return nullptr;
}

int grid::num_cells() const noexcept
{
    // Return the number of actually created cells, not the potential total
    return static_cast<int>(m_cells.size());
}

std::vector<std::shared_ptr<cell>> grid::get_cells() const noexcept
{
    // For large grids, this could be memory intensive
    // Only return actually created cells, not all potential cells
    std::vector<std::shared_ptr<cell>> cells;
    cells.reserve(m_cells.size());

    // Sort by index to maintain consistent ordering
    std::vector<std::pair<int, std::shared_ptr<cell>>> indexed_cells;
    indexed_cells.reserve(m_cells.size());

    for (const auto &[index, cell_ptr] : m_cells)
    {
        indexed_cells.emplace_back(index, cell_ptr);
    }

    std::sort(indexed_cells.begin(), indexed_cells.end(),
              [](const auto &a, const auto &b)
              {
                  return a.first < b.first;
              });

    for (const auto &[index, cell_ptr] : indexed_cells)
    {
        cells.emplace_back(cell_ptr);
    }

    return cells;
}

// Populate the vector of cells from the maze_adapter using natural ordering
void grid::sort(std::vector<std::shared_ptr<cell>> &cells) const noexcept
{
    // Use maze_adapter's built-in sorting capability
    std::lock_guard<std::mutex> lock(m_adapter_mutex);
    update_maze_adapter();

    auto sorted_adapter = m_maze_adapter.sort_by_index();

    cells.clear();
    cells.reserve(sorted_adapter.size());

    for (const auto &cell_ptr : sorted_adapter)
    {
        cells.emplace_back(cell_ptr);
    }
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

bool grid::set_cells(const std::vector<std::shared_ptr<cell>> &cells) noexcept
{
    // Clear existing cells
    m_cells.clear();
    m_topology.clear();

    // Add the cells to the grid
    for (const auto &cell_ptr : cells)
    {
        if (cell_ptr)
        {
            m_cells[cell_ptr->get_index()] = cell_ptr;
        }
    }

    // Update maze adapter
    {
        std::lock_guard<std::mutex> lock(m_adapter_mutex);
        m_maze_adapter = maze_adapter(cells);
    }

    // Build topology based on spatial relationships
    auto [rows, columns, levels] = m_dimensions;

    for (unsigned int l = 0; l < levels; ++l)
    {
        for (unsigned int r = 0; r < rows; ++r)
        {
            for (unsigned int c = 0; c < columns; ++c)
            {
                int cell_index = l * (rows * columns) + r * columns + c;
                std::unordered_map<Direction, int> neighbor_indices;

                // North neighbor
                if (r > 0)
                {
                    neighbor_indices[Direction::NORTH] = l * (rows * columns) + (r - 1) * columns + c;
                }

                // South neighbor
                if (r < rows - 1)
                {
                    neighbor_indices[Direction::SOUTH] = l * (rows * columns) + (r + 1) * columns + c;
                }

                // East neighbor
                if (c < columns - 1)
                {
                    neighbor_indices[Direction::EAST] = l * (rows * columns) + r * columns + (c + 1);
                }

                // West neighbor
                if (c > 0)
                {
                    neighbor_indices[Direction::WEST] = l * (rows * columns) + r * columns + (c - 1);
                }

                m_topology[cell_index] = neighbor_indices;
            }
        }
    }

    m_configured = true;
    return true;
}

void grid::set_str(std::string const &str) noexcept
{
    this->m_str = str;
}

std::string grid::get_str() const noexcept
{
    return this->m_str;
}

// Implementation of missing virtual methods from grid_operations

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

/// @brief Get the maze adapter for advanced cell operations
/// @return Const reference to the maze adapter
const maze_adapter &grid::get_maze_adapter() const noexcept
{
    std::lock_guard<std::mutex> lock(m_adapter_mutex);
    update_maze_adapter();
    return m_maze_adapter;
}

/// @brief Get a mutable maze adapter for advanced cell operations
/// @return Reference to the maze adapter
maze_adapter &grid::get_maze_adapter() noexcept
{
    std::lock_guard<std::mutex> lock(m_adapter_mutex);
    update_maze_adapter();
    return m_maze_adapter;
}

/// @brief Update the maze adapter with current cell data
void grid::update_maze_adapter() const
{
    // Convert cells map to vector for maze_adapter, sorted by index
    std::vector<std::shared_ptr<cell>> cells_vector;
    cells_vector.reserve(m_cells.size());

    // Create a vector of pairs to sort by index
    std::vector<std::pair<int, std::shared_ptr<cell>>> indexed_cells;
    indexed_cells.reserve(m_cells.size());

    for (const auto &[index, cell_ptr] : m_cells)
    {
        indexed_cells.emplace_back(index, cell_ptr);
    }

    // Sort by index to maintain consistent ordering
    std::sort(indexed_cells.begin(), indexed_cells.end(),
              [](const auto &a, const auto &b)
              {
                  return a.first < b.first;
              });

    // Extract the sorted cells
    for (const auto &[index, cell_ptr] : indexed_cells)
    {
        cells_vector.emplace_back(cell_ptr);
    }

    // Update the maze adapter
    m_maze_adapter = maze_adapter(std::move(cells_vector));
}
