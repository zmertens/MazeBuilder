#include <MazeBuilder/grid.h>

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <utility>
#include <functional>

#include <MazeBuilder/cell.h>

#if defined(MAZE_DEBUG)
#include <iostream>
#endif

using namespace mazes;

/// @brief 
/// @param rows 1
/// @param columns 1
/// @param height 1
grid::grid(unsigned int rows, unsigned int columns, unsigned int height)
    : grid::grid(std::make_tuple(rows, columns, height)) {
}

grid::grid(std::tuple<unsigned int, unsigned int, unsigned int> dimensions)
: m_dimensions(dimensions)
, m_calc_index{ [this](auto row, auto col)->int
    { return row * std::get<1>(this->m_dimensions) + col; } } {

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

/// @brief Inheritable configuration task
/// @return 
bool grid::get_future() noexcept {
    using namespace std;

    mt19937 rng{ 42681ul };
    static auto get_int = [&rng](int low, int high) ->int {
        uniform_int_distribution<int> dist{ low, high };
        return dist(rng);
        };

    vector<int> shuffled_indices;
    shuffled_indices.resize(get<0>(this->m_dimensions) * get<1>(this->m_dimensions));
    fill(shuffled_indices.begin(), shuffled_indices.end(), 0);
    unsigned int next_index{ 0 };
    for (auto itr{ shuffled_indices.begin() }; itr != shuffled_indices.end(); itr++) {
        *itr = next_index++;
    }
    shuffle(shuffled_indices.begin(), shuffled_indices.end(), rng);

    //return std::async(std::launch::async, [this, shuffled_indices]() mutable {
        this->build_fut(cref(shuffled_indices));

        return true;
        //});
}

void grid::build_fut(std::vector<int> const& indices) noexcept {
    using namespace std;

    //lock_guard<mutex> lock(m_config_mutex);

    auto [ROWS, COLUMNS, _] = this->get_dimensions();

    vector<shared_ptr<cell>> cells;
    cells.reserve(ROWS * COLUMNS);

    int row{ 0 }, column{ 0 }, index{ 0 };

    shared_ptr<cell> new_node = {};

    //while (row < ROWS && column < COLUMNS && index < ROWS * COLUMNS) {
    for (auto itr = indices.cbegin(); itr != indices.cend(); ++itr) {
        //int calc_index = this->m_calc_index(row, column);

        //if (calc_index < 0 || calc_index >= static_cast<int>(indices.size())) {
            //m_config_promise.set_value(false);
            //return;
        //}

        //index = indices.at(calc_index);
        index = *itr;

        new_node = make_shared<cell>(index);

        m_cells.insert({ index, new_node });

        cells.emplace_back(new_node);

        //if (m_binary_search_tree_root == nullptr) {
        //    m_binary_search_tree_root = new_node;
        //    cells.emplace_back(new_node->cell_ptr);
        //} else {
        //    // Insert method handles null root and checks
        //    this->insert(cref(new_node));
        //    //this->insert_recursive(ref(this->m_binary_search_tree_root), cref(new_node));
        //    cells.emplace_back(new_node->cell_ptr);
        //}

        // Insert method handles null root and checks
        //this->insert(cref(new_node));
        //this->insert_recursive(ref(this->m_binary_search_tree_root), cref(new_node));
        //cells.emplace_back(new_node->cell_ptr);

        /*auto new_count = this->count();
        if (new_count == current_cell_counter_from_root) {
#if defined(MAZE_DEBUG)
            cout << "BST insertion failed at count: " << new_count << endl;
#endif
        }*/

        //column = ++column % COLUMNS;
        //if (column == 0) {
        //    ++row;
        //}
    }

    this->configure_cells(std::ref(cells));

    //this->m_config_promise.set_value(true);
}


/// @brief Configure by nearest row, column pairing
/// @details A cell at (0, 0) will have a southern neighbor at (0, 1)
/// @details Counting is down top-left to right and then down (like an SQL table)
/// @param cells
void grid::configure_cells(std::vector<std::shared_ptr<cell>>& cells) const noexcept {
    using namespace std;

    auto [rows, columns, _] = this->m_dimensions;

    unsigned int rowCounter = 0, columnCounter = 0;
    unsigned int next_index = 0;
    while (rowCounter < rows && columnCounter < columns && next_index < rows * columns) {
        next_index = this->m_calc_index(rowCounter, columnCounter);
        auto&& cell = cells.at(next_index);

        if (rowCounter - 1 >= 0 && rowCounter - 1 < rows) {
            next_index = this->m_calc_index(rowCounter - 1, columnCounter);
            auto&& found = cells.at(next_index);
            if (found != nullptr)
                cell->set_north(found);
        }
        if (rowCounter + 1 < rows) {
            next_index = this->m_calc_index(rowCounter + 1, columnCounter);
            auto&& found = cells.at(next_index);
            if (found != nullptr)
                cell->set_south(found);
        }
        if (columnCounter - 1 >= 0 && columnCounter - 1 < columns) {
            next_index = this->m_calc_index(rowCounter, columnCounter - 1);
            auto&& found = cells.at(next_index);
            if (found != nullptr)
                cell->set_west(found);
        }
        if (columnCounter + 1 < columns) {
            next_index = this->m_calc_index(rowCounter, columnCounter + 1);
            auto&& found = cells.at(next_index);
            if (found != nullptr)
                cell->set_east(found);
        }
        columnCounter = ++columnCounter % columns;
        // check if there's a new row
        if (columnCounter == 0) {
            ++rowCounter;
        }
    } // while
}

std::tuple<unsigned int, unsigned int, unsigned int> grid::get_dimensions() const noexcept {
    return this->m_dimensions;
}

/// @brief Private implementation for inserting new nodes
/// @param parent 
/// @param new_node 
//void grid::insert(std::shared_ptr<node> const& new_node) noexcept {
//    using namespace std;

    //if (!m_binary_search_tree_root) {
    //    m_binary_search_tree_root = new_node;
    //    return;
    //}

    //auto current = m_binary_search_tree_root;

    //while (current) {
    //    if (new_node->cell_ptr->get_index() < current->cell_ptr->get_index()) {
    //        if (!current->left) {
    //            current->left = new_node;
    //            return;
    //        }
    //        current = current->left;
    //    } else if (new_node->cell_ptr->get_index() > current->cell_ptr->get_index()) {
    //        if (!current->right) {
    //            current->right = new_node;
    //            return;
    //        }
    //        current = current->right;
    //    } else {
    //        // Handle the case where the indices are equal
    //        return;
    //    }
    //}
//}

std::shared_ptr<cell> grid::search(int index) const noexcept {
    auto itr = m_cells.find(index);

    return (itr != m_cells.cend()) ? itr->second : nullptr;
}

int grid::count() const noexcept {
    return static_cast<int>(m_cells.size());
}

// Populate the vector of cells from the BST using natural ordering
void grid::to_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept {
    //std::call_once(m_config_flag, [this]() {
    //    m_config_promise.get_future().wait();
    //    });

    //// Populate the cells starting from the root
    //this->presort(this->m_binary_search_tree_root, ref(cells));

    for (auto&& [_, c] : m_cells) {
        cells.emplace_back(c);
    }
}

void grid::to_vec2(std::vector<std::vector<std::shared_ptr<cell>>>& cells) const noexcept {
    // Ensure the grid is configured
    //std::call_once(m_config_flag, [this]() {
    //    m_config_promise.get_future().wait();
    //    });

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
