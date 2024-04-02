#include "grid.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <random>

#include "cell.h"

using namespace mazes;
using namespace std;

/**
 * The grid constructor does several things: init members, create shuffled indices
 * so the binary search tree can be "more balanced" than a BST with nodes inserted in sorted order
 * and, sorts the created grid by row, column. Finally, it configures the N,S,E,W neighboring cells
 * @brief grid::grid
 * @param rows
 * @param columns
 * @param height = 0
 */
grid::grid(unsigned int rows, unsigned int columns, unsigned int height)
: m_rows{rows}
, m_columns{columns}
, m_height{height}
, m_binary_search_tree_root{nullptr}
, m_sort_by_row_column{[](shared_ptr<cell> const& c1, shared_ptr<cell> const& c2)->bool {
        if (c1->get_row() == c2->get_row()) {
            if (c1->get_column() == c2->get_column()) {
                return false;
            }
            return (c1->get_column() < c2->get_column()) ? true : false;
        }
        return (c1->get_row() < c2->get_row()) ? true : false; }}
, m_calc_index{[this](unsigned int row, unsigned int col)->unsigned int 
    {return row * this->m_columns + col;}} {
    
    vector<unsigned int> shuffled_indices;
    shuffled_indices.resize(rows * columns);
    fill(shuffled_indices.begin(), shuffled_indices.end(), 0);
    unsigned int next_index {0};
    for (auto itr {shuffled_indices.begin()}; itr != shuffled_indices.end(); itr++) {
        *itr = next_index++;
    }

    auto rd = std::random_device {}; 
    auto rng = std::default_random_engine { rd() };
    shuffle(begin(shuffled_indices), end(shuffled_indices), rng);    

    bool success = this->create_binary_search_tree(ref(shuffled_indices));
    if (success) {
        // Use a lambda function for sorting by row, column
        // First sort cells by row then column
        vector<shared_ptr<cell>> sorted_cells;
        sorted_cells.reserve(this->m_rows * this->m_columns);
        // populate a vector of cells from the grid (doesn't matter if it's sorted here, just need it filled)
        this->populate_vec(ref(sorted_cells));
        this->sort_by_row_then_col(ref(sorted_cells));
        configure_cells(ref(sorted_cells));
    }
}

bool grid::create_binary_search_tree(const std::vector<unsigned int>& shuffled_indices) {
    unsigned int row { 0 }, column { 0 };
    unsigned int index { 0 };

    while (row < this->m_rows && column < this->m_columns && index < this->m_rows * this->m_columns) {
        index = this->m_calc_index(row, column);
        index = shuffled_indices.at(index);
        // Check if the root hasn't been created
        if (this->m_binary_search_tree_root == nullptr) {
            this->m_binary_search_tree_root = {make_shared<cell>(row, column, index)};
        } else {
            this->insert(ref(this->m_binary_search_tree_root), row, column, index);
        }

        column = ++column % this->m_columns;
        // check if there's a new row
        if (column == 0) {
            ++row;
        }
    } // while

    return true;
}

/*
* Configure by nearest row, column pairing
* A cell at (0, 0) will have a southern neighbor at (0, 1)
* Counting is down top-left to right and then down (like an SQL table)
* @param cells are sorted by row and then column
*/
void grid::configure_cells(std::vector<std::shared_ptr<cell>>& cells) noexcept {
    unsigned int rowCounter = 0, columnCounter = 0;
    unsigned int next_index = 0;
    while (rowCounter < this->m_rows && columnCounter < this->m_columns && next_index < this->m_rows * this->m_columns) {
        next_index = this->m_calc_index(rowCounter, columnCounter);
        auto&& cell = cells.at(next_index);
        int row = cell->get_row();
        int column = cell->get_column();
        if (row - 1 >= 0 && row - 1 < this->m_rows) {
            next_index = this->m_calc_index(row - 1, column);
            auto&& found = cells.at(next_index);
            if (found != nullptr)
                cell->set_north(found);
        }
        if (row + 1 < this->m_rows) {
            next_index = this->m_calc_index(row + 1, column);
            auto&& found = cells.at(next_index);
            if (found != nullptr)
                cell->set_south(found);
        }
        if (column - 1 >= 0 && column - 1 < this->m_columns) {
            next_index = this->m_calc_index(row, column - 1);
            auto&& found = cells.at(next_index);
            if (found != nullptr)
                cell->set_west(found);
        }
        if (column + 1 < this->m_columns) {
            next_index = this->m_calc_index(row, column + 1);
            auto&& found = cells.at(next_index);
            if (found != nullptr)
                cell->set_east(found);
        }
        columnCounter = ++columnCounter % this->m_columns;
        // check if there's a new row
        if (columnCounter == 0) {
            ++rowCounter;
        }
    } // while
} // configure_cells

unsigned int grid::get_rows() const noexcept {
    return this->m_rows;
}

unsigned int grid::get_columns() const noexcept {
    return this->m_columns;
}

unsigned int grid::get_height() const noexcept {
    return this->m_height;
}

/**
 * @param max = 0
*/
unsigned int grid::max_index(shared_ptr<mazes::cell> const& parent, unsigned int max) const noexcept {
    if (parent != nullptr) {
        max_index(parent->get_left(), max);
        max_index(parent->get_right(), max);
        max = parent->get_index() > max ? max : parent->get_index();
    }
    return max;
}

/**
 * @param min = 0
*/
unsigned int grid::min_index(std::shared_ptr<cell> const& parent, unsigned int min) const noexcept {
    if (parent != nullptr) {
        min = parent->get_index() < min ? min : parent->get_index();
        max_index(parent->get_left(), min);
        max_index(parent->get_right(), min);
    }
    return min;
}

shared_ptr<cell> grid::get_root() const noexcept {
    return this->m_binary_search_tree_root;
}

/**
* Populate (instantiate a linear vector of cells using the data in the grid)
*/
void grid::populate_vec(std::vector<std::shared_ptr<cell>>& _cells) noexcept {
    this->sort(this->get_root(), ref(_cells));
}

/**
 * Iterate through the other_grid and insert to the current grid's root
 * Increment other_grid's indices 'i' by this current grid's max + 'i'
*/
void grid::grow(std::unique_ptr<grid> const& other_grid) noexcept {

}

/**
 * Keep calling insert recursively until we hit null (a leaf)
*/
void grid::insert(std::shared_ptr<cell> const& parent, unsigned int row, unsigned int col, unsigned int index) {
    if (parent->get_index() > index) {
        if (parent->get_left() == nullptr) {
            parent->set_left({make_shared<cell>(row, col, index)});
        } else {
            this->insert(parent->get_left(), row, col, index);
        }
    } else if (parent->get_index() < index) {
        if (parent->get_right() == nullptr) {
            parent->set_right({make_shared<cell>(row, col, index)});
        } else {
            this->insert(parent->get_right(), row, col, index);
        }
    }
}

shared_ptr<cell> grid::search(std::shared_ptr<cell> const& start, unsigned int index) const noexcept {
    if (start == nullptr || start->get_index() == index) {
        return start;
    } else if (start->get_index() > index) {
        return search(start->get_left(), index);
    } else {
        return search(start->get_right(), index);
    }
}

/**
* Find Dijkstra's shortest path
*/
bool grid::is_solveable() const noexcept {
    return false;
}

void grid::sort(std::shared_ptr<cell> const& parent, std::vector<std::shared_ptr<cell>>& cells_to_sort) noexcept {
    if (parent != nullptr) {
        this->sort(parent->get_left(), ref(cells_to_sort));
        cells_to_sort.emplace_back(parent);
        this->sort(parent->get_right(), ref(cells_to_sort));
    }
}

/**
* Sort the grid as if it were 2-dimensional grid and not hidden by a BST
* Compare rows, then if equal, compare columns
*/
void grid::sort_by_row_then_col(std::vector<std::shared_ptr<cell>>& cells_to_sort) noexcept {
    // now use STL sort by row, column with custom lambda function
    std::sort(cells_to_sort.begin(), cells_to_sort.end(), this->m_sort_by_row_column);
}