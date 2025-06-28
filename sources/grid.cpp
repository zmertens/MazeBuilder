#include <MazeBuilder/grid.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/lab.h>

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
    : grid::grid(std::make_tuple(rows, columns, height)) {
}

/// @brief 
/// @param dimensions 
grid::grid(std::tuple<unsigned int, unsigned int, unsigned int> dimens)
: m_dimensions(dimens)
, m_calculate_cell_index{ [this](auto row, auto col)->int
    { return row * std::get<1>(this->m_dimensions) + col; } }
, m_configured{ false } {

}

// Copy constructor
grid::grid(const grid& other)
: m_dimensions(other.m_dimensions) {
}

// Copy assignment operator
grid& grid::operator=(const grid& other) {
    if (this == &other) {
        return *this;
    }
    m_dimensions = other.m_dimensions;
    // Copy other members if necessary
    return *this;
}

// Move constructor
grid::grid(grid&& other) noexcept
: m_dimensions(other.m_dimensions) {
    // Move other members if necessary
}

// Move assignment operator
grid& grid::operator=(grid&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    m_dimensions = other.m_dimensions;
    return *this;
}

// Destructor
grid::~grid() {
    // First clean up cell references
    clear_cells();

    m_configured = false;
    m_dimensions = { 0, 0, 0 };
    m_calculate_cell_index = {};
}

std::shared_ptr<cell> grid::get_neighbor(const std::shared_ptr<cell>& c, Direction dir) const noexcept {
    if (!c) return nullptr;

    int cell_index = c->get_index();

    std::lock_guard<std::mutex> lock(m_topology_mutex);

    auto it_cell = m_topology.find(cell_index);
    if (it_cell == m_topology.end()) {
        return nullptr;
    }

    auto& dir_map = it_cell->second;
    auto it_dir = dir_map.find(dir);
    if (it_dir == dir_map.end()) {
        return nullptr;
    }

    int neighbor_index = it_dir->second;

    return search(neighbor_index);
}

void grid::set_neighbor(const std::shared_ptr<cell>& c, Direction dir, const std::shared_ptr<cell>& neighbor) noexcept {
    if (!c || !neighbor) return;

    int cell_index = c->get_index();
    int neighbor_index = neighbor->get_index();

    std::lock_guard<std::mutex> lock(m_topology_mutex);

    // Set the primary direction
    m_topology[cell_index][dir] = neighbor_index;

    // Set the reverse direction WITHOUT calling set_neighbor recursively
    Direction reverse_dir;
    switch (dir) {
    case Direction::NORTH:
        reverse_dir = Direction::SOUTH;
        break;
    case Direction::SOUTH:
        reverse_dir = Direction::NORTH;
        break;
    case Direction::EAST:
        reverse_dir = Direction::WEST;
        break;
    case Direction::WEST:
        reverse_dir = Direction::EAST;
        break;
    default:
        return; // Unknown direction
    }

    // Directly set the reverse mapping in the topology map
    m_topology[neighbor_index][reverse_dir] = cell_index;
}

std::vector<std::shared_ptr<cell>> grid::get_neighbors(const std::shared_ptr<cell>& c) const noexcept {
    std::vector<std::shared_ptr<cell>> neighbors;
    if (!c) return neighbors;

    neighbors.reserve(static_cast<size_t>(Direction::COUNT));

    for (size_t i = 0; i < static_cast<size_t>(Direction::COUNT); ++i) {
        Direction dir = static_cast<Direction>(i);
        if (auto neighbor = get_neighbor(c, dir)) {
            neighbors.push_back(neighbor);
        }
    }

    return neighbors;
}

void grid::clear_cells() noexcept {
    // First, clear topology information
    {
        std::lock_guard<std::mutex> lock(m_topology_mutex);
        m_topology.clear();
    }

    // Break all links between cells
    for (auto& [_, c] : m_cells) {
        if (c) {
            // Unlink all links - this should remove all connections between cells
            auto links = c->get_links();
            for (auto& [linked_cell, _] : links) {
                if (linked_cell) {
                    // Use remove_link which doesn't try to modify the other cell's links
                    c->remove_link(linked_cell);
                }
            }

            // Extra cleanup
            c->cleanup_links();
        }
    }

    // After breaking connections, clear the container
    m_cells.clear();

    // Complete reset - swap with an empty map to ensure memory is released
    std::unordered_map<int, std::shared_ptr<cell>>().swap(m_cells);
}

std::shared_ptr<cell> grid::get_north(const std::shared_ptr<cell>& c) const noexcept {
    return get_neighbor(c, Direction::NORTH);
}

std::shared_ptr<cell> grid::get_south(const std::shared_ptr<cell>& c) const noexcept {
    return get_neighbor(c, Direction::SOUTH);
}

std::shared_ptr<cell> grid::get_east(const std::shared_ptr<cell>& c) const noexcept {
    return get_neighbor(c, Direction::EAST);
}

std::shared_ptr<cell> grid::get_west(const std::shared_ptr<cell>& c) const noexcept {
    return get_neighbor(c, Direction::WEST);
}

std::tuple<unsigned int, unsigned int, unsigned int> grid::get_dimensions() const noexcept {

    return this->m_dimensions;
}

std::shared_ptr<cell> grid::search(int index) const noexcept {
    auto itr = m_cells.find(index);

    return (itr != m_cells.cend()) ? itr->second : nullptr;
}

int grid::num_cells() const noexcept {
    return static_cast<unsigned int>(m_cells.size());
}

std::vector<std::shared_ptr<cell>> grid::get_cells() const noexcept {

    std::vector<std::shared_ptr<cell>> cells;

    cells.reserve(m_cells.size());

    for (const auto& [_, c] : m_cells) {

        cells.emplace_back(c);
    }

    return cells;
}

// Populate the vector of cells from the BST using natural ordering
void grid::sort(std::vector<std::shared_ptr<cell>>& cells) const noexcept {

    std::sort(cells.begin(), cells.end(), [](auto c1, auto c2) {

        return c1->get_index() < c2->get_index();
        });
}

// Get the contents of a cell for this type of grid
std::string grid::contents_of(std::shared_ptr<cell> const& c) const noexcept {

    return " ";
}

// Get the background color for this type of grid
std::uint32_t grid::background_color_for(std::shared_ptr<cell> const& c) const noexcept {

    return { 0xFFFFFFFF };
}

grid_operations& grid::operations() noexcept {

    return *this;
}

const grid_operations& grid::operations() const noexcept {

    return *this;
}

bool grid::set_cells(const std::vector<std::shared_ptr<cell>>& cells) noexcept {
    // Clear existing cells
    m_cells.clear();
    m_topology.clear();
    
    // Add the cells to the grid
    for (const auto& cell_ptr : cells) {
        if (cell_ptr) {
            m_cells[cell_ptr->get_index()] = cell_ptr;
        }
    }
    
    // Build topology based on spatial relationships
    auto [rows, columns, levels] = m_dimensions;
    
    for (unsigned int l = 0; l < levels; ++l) {
        for (unsigned int r = 0; r < rows; ++r) {
            for (unsigned int c = 0; c < columns; ++c) {
                int cell_index = l * (rows * columns) + r * columns + c;
                std::unordered_map<Direction, int> neighbor_indices;
                
                // North neighbor
                if (r > 0) {
                    neighbor_indices[Direction::NORTH] = l * (rows * columns) + (r - 1) * columns + c;
                }
                
                // South neighbor
                if (r < rows - 1) {
                    neighbor_indices[Direction::SOUTH] = l * (rows * columns) + (r + 1) * columns + c;
                }
                
                // East neighbor
                if (c < columns - 1) {
                    neighbor_indices[Direction::EAST] = l * (rows * columns) + r * columns + (c + 1);
                }
                
                // West neighbor
                if (c > 0) {
                    neighbor_indices[Direction::WEST] = l * (rows * columns) + r * columns + (c - 1);
                }
                
                m_topology[cell_index] = neighbor_indices;
            }
        }
    }
    
    m_configured = true;
    return true;
}

void grid::set_str(std::string const& str) noexcept {
    this->m_str = str;
}

std::string grid::get_str() const noexcept {
    
    return this->m_str;
}
