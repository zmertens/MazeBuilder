#include <MazeBuilder/grid.h>

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <tuple>
#include <functional>

#include <MazeBuilder/cell.h>

#if defined(MAZE_DEBUG)
#include <iostream>
#endif
#include <numeric>

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
    // Clean up resources if necessary
    for (auto&& [_, c] : m_cells) {
        c.reset();
    }
}

void grid::notify_observers() noexcept {
    std::lock_guard<std::mutex> lock(m_observers_mutex);
    auto success{ false };
    for (const auto& observer : m_observers) {
        success = observer();
    }
    m_configured = success;
}

void grid::register_observer(std::function<bool(void)> const& observer) noexcept {
    std::lock_guard<std::mutex> lock(m_observers_mutex);
    m_observers.emplace_back(observer);
}

bool grid::is_observed() noexcept {
    return m_configured;
}

void grid::start_configuration(std::vector<int> const& indices) noexcept {
    using namespace std;

    if (m_configured) {

        return;
    }

    auto [ROWS, COLUMNS, _] = this->get_dimensions();

    vector<shared_ptr<cell>> cells;
    cells.reserve(ROWS * COLUMNS);

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
        m_cells.insert({ c->get_index(), c });
    }

    if (!m_observers.empty()) {
        notify_observers();
    }
}

/// @brief Configure by nearest row, column pairing
/// @details A cell at (0, 0) will have a southern neighbor at (0, 1)
/// @details Counting is down top-left to right and then down (like an SQL table)
/// @param cells
void grid::configure_cells(std::vector<std::shared_ptr<cell>>& cells) const noexcept {
    using namespace std;
    auto [ROWS, COLUMNS, _] = this->m_dimensions;

    for (unsigned int row = 0; row < ROWS; ++row) {
        for (unsigned int col = 0; col < COLUMNS; ++col) {
            int index = this->m_calc_index(row, col);

            if (index > cells.size()) {
#if defined(MAZE_DEBUG)
                cerr << "Grid configuration failed at index: " << index << endl;
#endif
            }

            auto&& c = cells.at(index);

            // Set north neighbor
            if (row > 0) {
                int north_index = this->m_calc_index(row - 1, col);
                auto&& north_cell = cells.at(north_index);
                c->set_north(north_cell);
            }

            // Set south neighbor
            if (row < ROWS - 1) {
                int south_index = this->m_calc_index(row + 1, col);
                auto&& south_cell = cells.at(south_index);
                c->set_south(south_cell);
            }

            // Set west neighbor
            if (col > 0) {
                int west_index = this->m_calc_index(row, col - 1);
                auto&& west_cell = cells.at(west_index);
                c->set_west(west_cell);
            }

            // Set east neighbor
            if (col < COLUMNS - 1) {
                int east_index = this->m_calc_index(row, col + 1);
                auto&& east_cell = cells.at(east_index);
                c->set_east(east_cell);
            }
        }
    }
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

    sort(cells.begin(), cells.end(), [](std::shared_ptr<cell> const& c1, std::shared_ptr<cell> const& c2) {
        return c1->get_index() < c2->get_index();
        });
}

// Create a 2D representation of the grid
void grid::to_vec2(std::vector<std::vector<std::shared_ptr<cell>>>& cells) const noexcept {

    // Populate the cells starting from the root
    std::vector<std::shared_ptr<cell>> flat_cells;
    this->to_vec(ref(flat_cells));

    // Get the grid dimensions
    auto [rows, columns, _] = this->get_dimensions();

    // Resize the 2D vector to match the grid dimensions
    cells.resize(rows);
    for (auto& row : cells) {
        row.resize(columns);
    }

    // Fill the 2D vector with cells from the 1D vector
    for (unsigned int i = 0; i < rows; ++i) {
        for (unsigned int j = 0; j < columns; ++j) {
            auto index = this->m_calc_index(i, j);
            if (index < 0 || index >= flat_cells.size()) {
                return;
            }

            cells[i][j] = flat_cells[index];
        }
    }
}

// Get the contents of a cell for this type of grid
std::optional<std::string> grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
    return { " " };
}

// Get the background color for this type of grid
std::optional<std::uint32_t> grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
    return { 0xFFFFFFFF };
}
