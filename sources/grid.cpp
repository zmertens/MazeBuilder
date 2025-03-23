#include <MazeBuilder/grid.h>

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <utility>
#include <functional>

#include <MazeBuilder/cell.h>

#include <iostream>

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
, m_binary_search_tree_root{ nullptr }        
, m_sort_by_row_column{ [](std::shared_ptr<node> const& c1, std::shared_ptr<node> const& c2)->bool {
    if (c1->cell_ptr->get_index() < c2->cell_ptr->get_index()) {
        return true;
    } else if (c1->cell_ptr->get_index() == c2->cell_ptr->get_index()) {
        return c1->left->cell_ptr->get_index() < c2->left->cell_ptr->get_index();
    } else {
        return false;
    }} }
, m_calc_index{ [this](auto row, auto col)->int
    { return row * std::get<1>(this->m_dimensions) + col; } } {
    
}

// Copy constructor
grid::grid(const grid& other)
: m_dimensions(other.m_dimensions)
, m_binary_search_tree_root(other.m_binary_search_tree_root) {
}

// Copy assignment operator
grid& grid::operator=(const grid& other) {
    if (this == &other) {
        return *this;
    }
    m_binary_search_tree_root = (other.m_binary_search_tree_root == nullptr) ? nullptr : other.m_binary_search_tree_root;
    m_dimensions = other.m_dimensions;
    // Copy other members if necessary
    return *this;
}

// Move constructor
grid::grid(grid&& other) noexcept
: m_dimensions(other.m_dimensions) {
    // Move other members if necessary
    m_binary_search_tree_root = std::exchange(other.m_binary_search_tree_root, nullptr);
}

// Move assignment operator
grid& grid::operator=(grid&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    m_binary_search_tree_root = std::exchange(other.m_binary_search_tree_root, nullptr);
    m_dimensions = other.m_dimensions;
    return *this;
}

// Destructor
grid::~grid() {
    // Clean up resources if necessary
    m_binary_search_tree_root.reset();
}

/// @brief Inheritable configuration task
/// @return 
std::future<bool> grid::get_future() noexcept {
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

    return std::async(std::launch::async, [this, shuffled_indices]() mutable {
        this->configure_nodes(cref(shuffled_indices));

        return true;
        });
}

void grid::configure_nodes(std::vector<int> const& indices) noexcept {
    using namespace std;

    auto [ROWS, COLUMNS, _] = this->get_dimensions();

    vector<shared_ptr<cell>> cells;
    cells.reserve(ROWS * COLUMNS);

    int row{ 0 }, column{ 0 }, index{ 0 };

    while (row < ROWS && column < COLUMNS && index < ROWS * COLUMNS) {
        int calc_index = this->m_calc_index(row, column);
        if (calc_index < 0 || calc_index >= static_cast<int>(indices.size())) {
            m_config_promise.set_value(false);
            return;
        }

        index = indices.at(calc_index);

        shared_ptr<node> new_node;

        if (this->m_binary_search_tree_root == nullptr) {
            this->m_binary_search_tree_root = make_shared<node>(index);
            new_node = this->m_binary_search_tree_root;
            cells.emplace_back(new_node->cell_ptr);
        } else {
            new_node = make_shared<node>(index);
            this->insert(ref(this->m_binary_search_tree_root), new_node);
            cells.emplace_back(new_node->cell_ptr);
        }

        column = ++column % COLUMNS;
        if (column == 0) {
            ++row;
        }
    }

    this->configure_cells(std::ref(cells));

    this->m_config_promise.set_value(true);
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
void grid::insert(std::shared_ptr<node>& parent, std::shared_ptr<node>& new_node) noexcept {
    if (!parent) {
        parent = new_node;
        return;
    }

    if (parent->cell_ptr->get_index() > new_node->cell_ptr->get_index()) {
        if (parent->left == nullptr) {
            parent->left = new_node;
        } else {
            this->insert(parent->left, new_node);
        }
    } else if (parent->cell_ptr->get_index() < new_node->cell_ptr->get_index()) {
        if (parent->right == nullptr) {
            parent->right = new_node;
        } else {
            this->insert(parent->right, new_node);
        }
    } else {
        // Index already exists in the BST, do nothing to avoid infinite recursion
        return;
    }
}

std::shared_ptr<cell> grid::search(int index) const noexcept {
    if (this->m_binary_search_tree_root == nullptr) {
        return nullptr;
    }
    auto node = this->search(this->m_binary_search_tree_root, index);
    if (node == nullptr) {
        return nullptr;
    }
    return node->cell_ptr;
}

template <typename Node>
std::shared_ptr<Node> grid::search(std::shared_ptr<Node> const& parent, int index) const noexcept {
    if (parent == nullptr || parent->cell_ptr->get_index() == index) {
        return parent;
    } else if (parent->cell_ptr->get_index() > index) {
        return search(parent->left, index);
    } else {
        return search(parent->right, index);
    }
}

// Populate the vector of cells from the BST using natural ordering
void grid::to_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept {
    std::call_once(m_config_flag, [this]() {
        m_config_promise.get_future().wait();
        });

    // Populate the cells starting from the root
    this->presort(this->m_binary_search_tree_root, ref(cells));
}

void grid::to_vec2(std::vector<std::vector<std::shared_ptr<cell>>>& cells) const noexcept {
    // NEED TO IMPLEMENT!!
}

void grid::inorder(std::shared_ptr<node> const& parent, std::vector<std::shared_ptr<cell>>& cells) const noexcept {
    if (parent != nullptr) {
        this->inorder(parent->left, ref(cells));
        cells.emplace_back(parent->cell_ptr);
        this->inorder(parent->right, ref(cells));
    }
}

void grid::presort(std::shared_ptr<node> const& parent, std::vector<std::shared_ptr<cell>>& cells) const noexcept {
    if (parent != nullptr) {
        cells.emplace_back(parent->cell_ptr);
        this->presort(parent->left, ref(cells));
        this->presort(parent->right, ref(cells));
    }
}

//
// Sort the grid as if it were 2-dimensional grid
// Compare rows, then if equal, compare columns
//
void grid::sort_by_row_then_col(std::vector<std::shared_ptr<node>>& nodes) const noexcept {
    // now use STL sort by row, column with custom lambda function
    std::sort(nodes.begin(), nodes.end(), this->m_sort_by_row_column);
}

// Get the contents of a cell for this type of grid
std::optional<std::string> grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
    return { " " };
}

// Get the background color for this type of grid
std::optional<std::uint32_t> grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
    return { 0xFFFFFFFF };
}
