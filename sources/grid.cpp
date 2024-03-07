#include "grid.h"

#include <iostream>
#include <string>
#include <stdexcept>

using namespace mazes;
using namespace std;

grid::grid(unsigned int rows, unsigned int columns)
: m_rows{rows}
, m_columns{columns}
, m_grid{} {
    this->prepare_grid(m_grid);
    this->configure_cells(m_grid);
    // this->print(this->m_grid);
}

void grid::prepare_grid(vector<vector<shared_ptr<cell>>>& grid) noexcept {
    for (auto row {0}; row < this->m_rows; row++) {
        vector<shared_ptr<cell>> new_row {};
        for (auto col {0}; col < this->m_columns; col++) {
            new_row.emplace_back(make_shared<cell>(row, col));
        }
        grid.emplace_back(new_row);
    }
}

void grid::configure_cells(vector<vector<shared_ptr<cell>>>& grid) noexcept {
    for (auto row {0}; row < grid.size(); ++row) {
        for (auto column {0}; column < grid.at(row).size(); column++) {
            auto&& temp = grid.at(row).at(column);
            if (temp != nullptr) {
                try {
                    
                    if (row - 1 >= 0)
                        temp->set_north(grid.at(row - 1).at(column));
                    if (row + 1 < grid.size())
                        temp->set_south(grid.at(row + 1).at(column));
                    if (column - 1 >= 0)
                        temp->set_west(grid.at(row).at(column - 1));
                    if (column + 1 < grid.at(row).size())
                        temp->set_east(grid.at(row).at(column + 1));
                } catch (std::out_of_range& ex) {
                    cerr << "ERROR: " << ex.what() << endl;
                }
            }
        }
    }
}

void grid::print_grid_cells() const noexcept {
    cout << "INFO: " << "Printing grid cells" << endl;
    for (auto row {0}; row < m_grid.size(); row++) {
        for (auto col {0}; col < m_grid.at(row).size(); col++) {
            cout << "INFO: Cell[" << row << "][" << col << "]:" << m_grid[row][col] << endl;
        }
    }
}

std::vector<shared_cell_ptr> grid::operator[](std::size_t i) {
    return this->m_grid.at(i);
}

std::vector<shared_cell_ptr> const& grid::operator[](std::size_t i) const {
    return this->m_grid.at(i);
}

std::vector<std::vector<std::shared_ptr<cell>>> grid::get_grid() const {
    return this->m_grid;
}

unsigned int grid::get_rows() const noexcept {
    return this->m_rows;
}

unsigned int grid::get_columns() const noexcept {
    return this->m_columns;
}
