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
, m_calc_index{[this](unsigned int row, unsigned int col)->int 
    {return row * this->m_columns + col;}} {
    
    vector<int> shuffled_indices;
    shuffled_indices.resize(rows * columns);
    fill(shuffled_indices.begin(), shuffled_indices.end(), 0);
    unsigned int next_index {0};
    for (auto itr {shuffled_indices.begin()}; itr != shuffled_indices.end(); itr++) {
        *itr = next_index++;
    }

    auto rd = std::random_device {}; 
    auto rng = std::default_random_engine { rd() };
    shuffle(begin(shuffled_indices), end(shuffled_indices), rng);    

    bool success = this->create_binary_search_tree(cref(shuffled_indices));
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

bool grid::create_binary_search_tree(const std::vector<int>& shuffled_indices) {
    unsigned int row { 0 }, column { 0 };
    int index { 0 };

    while (row < this->m_rows && column < this->m_columns && index < this->m_rows * this->m_columns) {
        index = this->m_calc_index(row, column);
        index = shuffled_indices.at(index);
        // Check if the root hasn't been created
        if (this->m_binary_search_tree_root == nullptr) {
            this->m_binary_search_tree_root = {make_shared<cell>(row, column, index)};
        } else {
            this->insert(ref(this->m_binary_search_tree_root), index);
			auto&& found = this->search(cref(this->m_binary_search_tree_root), index);
			if (found) {
				found->set_row(row);
				found->set_column(column);
            } else {
                return false;
            }
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
void grid::populate_vec(std::vector<std::shared_ptr<cell>>& _cells) const noexcept {
    this->sort(this->get_root(), ref(_cells));
}

/**
 * @brief Iterate through the other_grid and insert to the current grid's root
 */
void grid::append(std::unique_ptr<grid_interface> const& other_grid) noexcept {
    vector<shared_ptr<cell>> cells_to_sort;
	if (auto ptr = dynamic_cast<grid*>(other_grid.get())) {
        ptr->populate_vec(ref(cells_to_sort));
	}
    
    for (auto&& c : cells_to_sort) {
        this->insert(this->get_root(), c->get_index());
    }
}

/**
 * Keep calling insert recursively until we hit null (a leaf)
*/
void grid::insert(std::shared_ptr<cell> const& parent, int index) noexcept {
    if (parent->get_index() > index) {
        if (parent->get_left() == nullptr) {
            parent->set_left({make_shared<cell>(index)});
        } else {
            this->insert(parent->get_left(), index);
        }
    } else if (parent->get_index() < index) {
        if (parent->get_right() == nullptr) {
            parent->set_right({make_shared<cell>(index)});
        } else {
            this->insert(parent->get_right(), index);
        }
    }
}

/**
 * @brief Updating should also update row, col of the cell
 */
bool grid::update(std::shared_ptr<cell>& parent, int old_index, int new_index) noexcept {
    if (parent == nullptr) {
        return false;
    }

    auto found = search(parent, old_index);

	// Need to update indices, row, col, and update BST
    if (found) {
		found->set_index(new_index);
        unsigned int new_row = new_index / this->m_columns;
        unsigned int new_column = new_index % this->m_columns;
        found->set_row(new_row);
        found->set_column(new_column);

        auto cells = this->to_vec();
        vector<int> indices;
        indices.reserve(cells.size());
        transform(cells.cbegin(), cells.cend(), back_inserter(indices), 
            [](const shared_ptr<cell>& c) { return c->get_index(); });
        this->m_binary_search_tree_root.reset();
        indices.emplace_back(new_index);
		bool success = this->create_binary_search_tree(cref(indices));
        if (success) {
            this->populate_vec(ref(cells));
            this->sort_by_row_then_col(ref(cells));
            configure_cells(ref(cells));
        }
        return success;
	} else {
        return false;
    }
}

shared_ptr<cell> grid::search(std::shared_ptr<cell> const& start, int index) const noexcept {
    if (start == nullptr || start->get_index() == index) {
        return start;
    } else if (start->get_index() > index) {
        return search(start->get_left(), index);
    } else {
        return search(start->get_right(), index);
    }
}

void grid::del(std::shared_ptr<cell> parent, int index) noexcept {
    if (!parent) 
        return;

    if (index < parent->get_index()) {
        del(parent->get_left(), index);
    } else if (index > parent->get_index()) {
        del(parent->get_right(), index);
    } else {
        // Node to delete has no children
        if (parent->get_left() == nullptr && parent->get_right() == nullptr) {
            parent = nullptr;
        } else if (parent->get_left() == nullptr) {
            parent = parent->get_right();
        } else if (parent->get_right() == nullptr) {
            parent = parent->get_left();
        } else {
            auto min = min_index(parent->get_right());
			this->update(parent, parent->get_index(), min);
            del(parent->get_right(), min);
        }
    }
}

void grid::sort(std::shared_ptr<cell> const& parent, std::vector<std::shared_ptr<cell>>& cells_to_sort) const noexcept {
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
void grid::sort_by_row_then_col(std::vector<std::shared_ptr<cell>>& cells_to_sort) const noexcept {
    // now use STL sort by row, column with custom lambda function
    std::sort(cells_to_sort.begin(), cells_to_sort.end(), this->m_sort_by_row_column);
}

/**
 * @brief
 * @param cell_size = 25
 */
vector<uint8_t> grid::to_png(const unsigned int cell_size) const noexcept {
    int png_w = cell_size * this->get_columns();
    int png_h = cell_size * this->get_rows();
    // Init png data with white background
    vector<uint8_t> png_data((png_w + 1) * (png_h + 1) * 4, 255);

    // Black
    uint32_t wall_color = 0x000000FF;

    // Helper functions to populate png_data
    auto draw_rect = [&png_data, &png_w](int x1, int y1, int x2, int y2, uint32_t color) {
        for (int y = y1; y <= y2; ++y) {
            for (int x = x1; x <= x2; ++x) {
                int index = (y * (png_w + 1) + x) * 4;
                png_data[index] = (color >> 24) & 0xFF;
                png_data[index + 1] = (color >> 16) & 0xFF;
                png_data[index + 2] = (color >> 8) & 0xFF;
                png_data[index + 3] = color & 0xFF;
            }
        }
        };

    auto draw_line = [&png_data, &png_w](int x1, int y1, int x2, int y2, uint32_t color) {
        if (x1 == x2) {
            for (int y = y1; y <= y2; ++y) {
                int index = (y * (png_w + 1) + x1) * 4;
                png_data[index] = (color >> 24) & 0xFF;
                png_data[index + 1] = (color >> 16) & 0xFF;
                png_data[index + 2] = (color >> 8) & 0xFF;
                png_data[index + 3] = color & 0xFF;
            }
        } else if (y1 == y2) {
            for (int x = x1; x <= x2; ++x) {
                int index = (y1 * (png_w + 1) + x) * 4;
                png_data[index] = (color >> 24) & 0xFF;
                png_data[index + 1] = (color >> 16) & 0xFF;
                png_data[index + 2] = (color >> 8) & 0xFF;
                png_data[index + 3] = color & 0xFF;
            }
        }
        };

    vector<shared_ptr<cell>> cells;
    cells.reserve(this->get_rows() * this->get_columns());
    this->sort(this->get_root(), ref(cells));

    for (const auto& c : cells) {
        int x1 = c->get_column() * cell_size;
        int y1 = c->get_row() * cell_size;
        int x2 = (c->get_column() + 1) * cell_size;
        int y2 = (c->get_row() + 1) * cell_size;

        uint32_t color = this->background_color_for(c);

        draw_rect(x1, y1, x2, y2, color);

        if (!c->get_north()) {
            draw_line(x1, y1, x2, y1, wall_color);
        }
        if (!c->get_west()) {
            draw_line(x1, y1, x1, y2, wall_color);
        }
        if (!c->is_linked(c->get_east())) {
            draw_line(x2, y1, x2, y2, wall_color);
        }
        if (!c->is_linked(c->get_south())) {
            draw_line(x1, y2, x2, y2, wall_color);
        }
    }

    return png_data;
}

std::vector<std::shared_ptr<cell>> grid::to_vec() const noexcept {
	vector<shared_ptr<cell>> cells;
	cells.reserve(this->get_rows() * this->get_columns());
	this->populate_vec(ref(cells));
    return cells;
}

std::string grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
    return " ";
}

std::uint32_t grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
    return 0xFFFFFFFF;
}