#include <MazeBuilder/grid.h>

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <tuple>
#include <functional>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/lab.h>

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
, m_calc_index{ [this](auto row, auto col)->int
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
    m_calc_index = {};
}

void grid::start_configuration(std::vector<int> const& indices) noexcept {
    using namespace std;

    if (m_configured) {

        return;
    }

    vector<shared_ptr<cell>> cells;
    cells.reserve(indices.size());

    int row{ 0 }, column{ 0 }, index{ 0 }, last_cell_count{ 0 };

    shared_ptr<cell> new_node = {};

    for (auto itr = indices.cbegin(); itr != indices.cend(); ++itr) {

        index = *itr;

        new_node = make_shared<cell>(index);

        cells.emplace_back(new_node);

#if defined(MAZE_DEBUG)
        auto new_count = static_cast<int>(cells.size());
        if (new_count == last_cell_count) {

            cout << "Cell insertion failed at count: " << new_count << endl;

            break;
        }
#endif
    }

    sort(cells.begin(), cells.end(), [](auto c1, auto c2) {

        return c1->get_index() < c2->get_index();
        });

    this->configure_cells(std::ref(cells));

    for (const auto& c : cells) {
        m_cells.emplace(c->get_index(), c);
    }
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
    case Direction::North:
        reverse_dir = Direction::South;
        break;
    case Direction::South:
        reverse_dir = Direction::North;
        break;
    case Direction::East:
        reverse_dir = Direction::West;
        break;
    case Direction::West:
        reverse_dir = Direction::East;
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

    neighbors.reserve(static_cast<size_t>(Direction::Count));

    for (size_t i = 0; i < static_cast<size_t>(Direction::Count); ++i) {
        Direction dir = static_cast<Direction>(i);
        if (auto neighbor = get_neighbor(c, dir)) {
            neighbors.push_back(neighbor);
        }
    }

    return neighbors;
}

void grid::configure_cells(std::vector<std::shared_ptr<cell>>& cells) noexcept {
    using namespace std;
    auto [ROWS, COLUMNS, _] = this->m_dimensions;

    // Clear existing topology
    {
        std::lock_guard<std::mutex> lock(m_topology_mutex);
        m_topology.clear();
    }

    for (unsigned int row = 0; row < ROWS; ++row) {
        for (unsigned int col = 0; col < COLUMNS; ++col) {
            int index = this->m_calc_index(row, col);

            if (index >= static_cast<int>(cells.size())) {
                return;
            }

            auto c = cells.at(index);

            // We only need to set East and South neighbors
            // North and West will be set automatically through the bidirectional mechanism

            // Set east neighbor
            if (col < COLUMNS - 1) {
                auto east_index = this->m_calc_index(row, col + 1);
                if (east_index >= 0 && east_index < static_cast<int>(cells.size())) {
                    auto east_cell = cells.at(east_index);
                    set_neighbor(c, Direction::East, east_cell);
                }
            }

            // Set south neighbor
            if (row < ROWS - 1) {
                auto south_index = this->m_calc_index(row + 1, col);
                if (south_index >= 0 && south_index < static_cast<int>(cells.size())) {
                    auto south_cell = cells.at(south_index);
                    set_neighbor(c, Direction::South, south_cell);
                }
            }
        }
    }

    // Mark as configured
    m_configured = true;
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

// Populate the vector of cells from the BST using natural ordering
void grid::to_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept {

    for (auto&& [_, c] : m_cells) {
        cells.emplace_back(c);
    }

    sort(cells.begin(), cells.end(), [](auto c1, auto c2) {

        return c1->get_index() < c2->get_index();
        });
}

// Get the contents of a cell for this type of grid
std::optional<std::string> grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
    return { " " };
}

// Get the background color for this type of grid
std::optional<std::uint32_t> grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
    return { 0xFFFFFFFF };
}
