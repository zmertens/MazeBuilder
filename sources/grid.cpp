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
            auto temp {make_shared<cell>(row, col)};
            // cout << temp << endl;
            new_row.emplace_back(temp);
        }
        grid.emplace_back(new_row);
        // cout << grid.at(row).back() << endl;
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

void grid::print_grid_cells(vector<vector<shared_ptr<cell>>> const& grid) const noexcept {
    cout << "INFO: " << "Printing grid cells" << endl;
    for (auto row {0}; row < grid.size(); row++) {
        for (auto col {0}; col < grid.at(row).size(); col++) {
            cout << "INFO: Cell[" << row << "][" << col << "]:" << grid.at(row).at(col) << endl;
        }
    }
}

std::vector<std::vector<std::shared_ptr<cell>>> grid::get_grid() const {
    return this->m_grid;
}