#include <MazeBuilder/grid.h>

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <utility>
#include <functional>

#include <MazeBuilder/cell.h>

using namespace mazes;
using namespace std;

// Constructor
grid::grid(unsigned int rows, unsigned int columns, unsigned int height)
: m_dimensions{rows, columns, height}
, m_binary_search_tree_root{nullptr}
, m_sort_by_row_column{ [](shared_ptr<node> const& c1, shared_ptr<node> const& c2)->bool {
        if (c1->cell_ptr->get_index() < c2->cell_ptr->get_index()) {
            return true;
        } else if (c1->cell_ptr->get_index() == c2->cell_ptr->get_index()) {
            return c1->left->cell_ptr->get_index() < c2->left->cell_ptr->get_index();
        } else {
            return false;
        }} }
, m_calc_index{[this, &columns](auto row, auto col)->int 
    {return row * columns + col;}} {
    
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
        sorted_cells.reserve(rows * columns);
        // populate a vector of cells from the grid (doesn't matter if it's sorted here, just need it filled)
        this->to_vec(ref(sorted_cells));
        //this->sort_by_row_then_col(ref(sorted_cells));
        configure_cells(ref(sorted_cells));
    }
}

grid::grid(std::tuple<unsigned int, unsigned int, unsigned int> dimensions)
    : m_dimensions(dimensions) {
}

// Copy constructor
grid::grid(const grid& other)
    : m_binary_search_tree_root(other.m_binary_search_tree_root),
      m_dimensions(other.m_dimensions) {
    // Copy other members if necessary
}

// Copy assignment operator
grid& grid::operator=(const grid& other) {
    if (this == &other) {
        return *this;
    }
    m_binary_search_tree_root = other.m_binary_search_tree_root;
    m_dimensions = other.m_dimensions;
    // Copy other members if necessary
    return *this;
}

// Move constructor
grid::grid(grid&& other) noexcept
    : m_dimensions(move(other.m_dimensions)) {
    // Move other members if necessary

    m_binary_search_tree_root = std::exchange(other.m_binary_search_tree_root, nullptr);
}

// Move assignment operator
grid& grid::operator=(grid&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    m_binary_search_tree_root = std::exchange(other.m_binary_search_tree_root, nullptr);
    m_dimensions = move(other.m_dimensions);
    // Move other members if necessary
    return *this;
}

// Destructor
grid::~grid() {
    // Clean up resources if necessary
    m_binary_search_tree_root.reset();
}

bool grid::create_binary_search_tree(const std::vector<int>& shuffled_indices) {
    auto [rows, columns, _] = this->m_dimensions;
    int index { 0 }, row{0}, column{0};

    while (row < rows && column < columns && index < rows * columns) {
        index = this->m_calc_index(row, column);
        index = shuffled_indices.at(index);
        // Check if the root hasn't been created
        if (this->m_binary_search_tree_root == nullptr) {
            this->m_binary_search_tree_root = {make_shared<node>(index)};
        } else {
            this->insert(ref(this->m_binary_search_tree_root), index);
			//auto&& found = this->search(cref(this->m_binary_search_tree_root), index);
			//if (found) {
				//found->set_row(row);
				//found->set_column(column);
            //} else {
                //return false;
            //}
        }

        column = ++column % columns;
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
//void grid::configure_cells(std::vector<std::shared_ptr<cell>>& cells) noexcept {
//
//    auto [rows, columns, _] = this->m_dimensions;
//
//    unsigned int rowCounter = 0, columnCounter = 0;
//    unsigned int next_index = 0;
//    while (rowCounter < rows && columnCounter < columns && next_index < rows * columns) {
//        next_index = this->m_calc_index(rowCounter, columnCounter);
//        auto&& cell = cells.at(next_index);
//        int row = cell->get_row();
//        int column = cell->get_column();
//        if (row - 1 >= 0 && row - 1 < rows) {
//            next_index = this->m_calc_index(row - 1, column);
//            auto&& found = cells.at(next_index);
//            if (found != nullptr)
//                cell->set_north(found);
//        }
//        if (row + 1 < rows) {
//            next_index = this->m_calc_index(row + 1, column);
//            auto&& found = cells.at(next_index);
//            if (found != nullptr)
//                cell->set_south(found);
//        }
//        if (column - 1 >= 0 && column - 1 < columns) {
//            next_index = this->m_calc_index(row, column - 1);
//            auto&& found = cells.at(next_index);
//            if (found != nullptr)
//                cell->set_west(found);
//        }
//        if (column + 1 < columns) {
//            next_index = this->m_calc_index(row, column + 1);
//            auto&& found = cells.at(next_index);
//            if (found != nullptr)
//                cell->set_east(found);
//        }
//        columnCounter = ++columnCounter % columns;
//        // check if there's a new row
//        if (columnCounter == 0) {
//            ++rowCounter;
//        }
//    } // while
//} // configure_cells

void grid::configure_cells(std::vector<std::shared_ptr<cell>>& cells) noexcept {
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

/**
 * @brief Iterate through the other_grid and insert to the current grid's root
 */
//void grid::append(std::shared_ptr<grid_interface> const& other_grid) noexcept {
//    vector<shared_ptr<cell>> cells_to_sort;
//	if (auto ptr = dynamic_cast<grid*>(other_grid.get())) {
//        ptr->to_vec(ref(cells_to_sort));
//	}
//    
//    for (auto&& c : cells_to_sort) {
//        this->insert(this->m_binary_search_tree_root, c->get_index());
//    }
//}

/// @brief Public implementation of insert
/// @param parent 
/// @param index 
//void grid::insert(std::shared_ptr<cell> const& parent, int index) noexcept {
//    auto&& found = this->search(index);
//}

/// @brief Private implementation of insert
/// @param parent 
/// @param index 
void grid::insert(std::shared_ptr<node> parent, int index) noexcept {
    if (!parent) {
        return;
    }

    if (parent->cell_ptr->get_index() > index) {
        if (parent->left == nullptr) {
            parent->left = std::make_shared<node>(index);
        } else {
            this->insert(parent->left, index);
        }
    } else if (parent->cell_ptr->get_index() < index) {
        if (parent->right == nullptr) {
            parent->right = std::make_shared<node>(index);
        } else {
            this->insert(parent->right, index);
        }
    } else {
        // Index already exists in the BST, do nothing to avoid infinite recursion
        return;
    }
}

//void grid::insert(std::shared_ptr<cell> const& parent, int index) noexcept {
//    if (!parent) {
//        return;
//    }
//
//    if (parent->get_index() > index) {
//        if (parent->get_left() == nullptr) {
//            parent->set_left(make_shared<cell>(index));
//        } else {
//            this->insert(parent->get_left(), index);
//        }
//    } else if (parent->get_index() < index) {
//        if (parent->get_right() == nullptr) {
//            parent->set_right(make_shared<cell>(index));
//        } else {
//            this->insert(parent->get_right(), index);
//        }
//    } else {
//        // Index already exists in the BST, do nothing to avoid infinite recursion
//        return;
//    }
//}

/**
 * @brief Updating should also update row, col of the cell
 */
//bool grid::update(std::shared_ptr<cell>& parent, int old_index, int new_index) noexcept {
//    if (parent == nullptr) {
//        return false;
//    }
//
//    auto found = search(parent, old_index);
//
//	// Need to update indices, row, col, and update BST
//    if (found) {
//        auto [rows, columns, _] = this->m_dimensions;
//
//		found->set_index(new_index);
//        unsigned int new_row = new_index / columns;
//        unsigned int new_column = new_index % columns;
//        found->set_row(new_row);
//        found->set_column(new_column);
//
//        vector<shared_ptr<cell>> cells;
//		cells.reserve(rows * columns);
//        this->to_vec(ref(cells));
//        vector<int> indices;
//        indices.reserve(cells.size());
//        transform(cells.cbegin(), cells.cend(), back_inserter(indices), 
//            [](const shared_ptr<cell>& c) { return c->get_index(); });
//        this->m_binary_search_tree_root.reset();
//        indices.emplace_back(new_index);
//		bool success = this->create_binary_search_tree(cref(indices));
//        if (success) {
//            this->to_vec(ref(cells));
//            this->sort_by_row_then_col(ref(cells));
//            configure_cells(ref(cells));
//        }
//        return success;
//	} else {
//        return false;
//    }
//}

shared_ptr<cell> grid::search(int index) const noexcept {
    return this->search(this->m_binary_search_tree_root, index)->cell_ptr;
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

//// NOT IMPLEMENTED !!
//void grid::del(std::shared_ptr<cell> parent, int index) noexcept {
//    if (!parent) 
//        return;
//
//    // if (index < parent->get_index()) {
//    //     del(parent->get_left(), index);
//    // } else if (index > parent->get_index()) {
//    //     del(parent->get_right(), index);
//    // } else {
//    //     // Node to delete has no children
//    //     if (parent->get_left() == nullptr && parent->get_right() == nullptr) {
//    //         parent = nullptr;
//    //     } else if (parent->get_left() == nullptr) {
//    //         parent = parent->get_right();
//    //     } else if (parent->get_right() == nullptr) {
//    //         parent = parent->get_left();
//    //     } else {
//    //         // auto min = min_index(parent->get_right());
//	// 		// this->update(parent, parent->get_index(), min);
//    //         // del(parent->get_right(), min);
//    //     }
//    // }
//}
//
// Populate the vector of cells with the BST
void grid::to_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept {
    // First populate the cells from the grid
    this->presort(this->m_binary_search_tree_root, ref(cells));
    // Sort the cells by row then column
    //this->sort_by_row_then_col(ref(cells));
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
optional<std::string> grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
    return { " " };
}

// Get the background color for this type of grid
optional<std::uint32_t> grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
    return { 0xFFFFFFFF };
}
