/**
 * The grid class implements a Binary Search Tree for performance and API reasons
 * Grid represents an ASCII-format maze in 2D, or Wavefront object file in 3D
*/

#ifndef GRID_H
#define GRID_H

#include <vector>
#include <memory>
#include <sstream>
#include <string>
#include <functional>
#include <algorithm>

#include "cell.h"
#include "maze_types_enum.h"

namespace mazes {

class grid {
public:
    grid(unsigned int rows, unsigned int columns, unsigned int height = 0);

    unsigned int get_rows() const noexcept;
    unsigned int get_columns() const noexcept;
    unsigned int get_height() const noexcept;
    unsigned int max_index(std::shared_ptr<cell> const& parent, unsigned int max = 0) const noexcept;
    unsigned int min_index(std::shared_ptr<cell> const& parent, unsigned int min = 0) const noexcept;
    std::shared_ptr<cell> get_root() const noexcept;
    void populate_vec(std::vector<std::shared_ptr<cell>>& _cells) noexcept;
    void grow(std::unique_ptr<grid> const& other_grid) noexcept;
    void insert(std::shared_ptr<cell> const& parent, unsigned int row, unsigned int col, unsigned int index);
    std::shared_ptr<cell> search(std::shared_ptr<cell> const& start, unsigned int index) const noexcept;
    bool is_solveable() const noexcept;
    // sort ascending per index-value
    void sort(std::shared_ptr<cell> const& parent, std::vector<std::shared_ptr<cell>>& cells_to_sort) noexcept;
    void sort_by_row_then_col(std::vector<std::shared_ptr<cell>>& cells_to_sort) noexcept;
private:

    bool create_binary_search_tree(const std::vector<unsigned int>& shuffled_indices);
    void configure_cells(std::vector<std::shared_ptr<cell>>& cells) noexcept;

    friend std::ostream& operator<<(std::ostream& os, grid& g) {
        // First sort cells by row then column
        std::vector<std::shared_ptr<cell>> cells;
        cells.reserve(g.get_rows() * g.get_columns());
        // populate the cells with the BST
        g.sort(g.get_root(), ref(cells));
        g.sort_by_row_then_col(ref(cells));

        // ---+
        static constexpr auto barrier = { MAZE_BARRIER2, MAZE_BARRIER2, MAZE_BARRIER2, MAZE_CORNER };
        static const std::string wall_plus_corner{ barrier };
        std::stringstream output;
        output << MAZE_CORNER;
        for (auto i{ 0u }; i < g.get_columns(); i++) {
            output << wall_plus_corner;
        }
        output << "\n";

        auto rowCounter {0u}, columnCounter {0u};
        while (rowCounter < g.get_rows()) {
            std::stringstream top_builder, bottom_builder;
            top_builder << MAZE_BARRIER1;
            bottom_builder << MAZE_CORNER;
            while (columnCounter < g.get_columns()) {
                auto next_index {rowCounter * g.get_columns() + columnCounter};
                if (next_index < cells.size()) {
                    auto&& temp = cells.at(next_index);
                    // bottom left cell needs boundaries
                    if (temp == nullptr)
                        temp = { std::make_shared<cell>(-1, -1, next_index) };
                    // 3 spaces in body
                    std::string body = "   ";
                    static const std::string vertical_barrier_str{ MAZE_BARRIER1 };
                    std::string east_boundary = temp->is_linked(temp->get_east()) ? " " : vertical_barrier_str;
                    top_builder << body << east_boundary;
                    std::string south_boundary = temp->is_linked(temp->get_south()) ? "   " : wall_plus_corner.substr(0, wall_plus_corner.size() - 1);
                    bottom_builder << south_boundary << "+";
                    columnCounter++;
                }
            }
            columnCounter = 0;
            rowCounter++;
            output << top_builder.str() << "\n" << bottom_builder.str() << "\n";
        } // while
        os << output.str() << std::endl;

        return os;
    } // << operator
    std::function<int(std::shared_ptr<cell> const&, std::shared_ptr<cell> const&)> m_sort_by_row_column;
    std::function<unsigned int(unsigned int, unsigned int)> m_calc_index;
    std::shared_ptr<cell> m_binary_search_tree_root;
    const unsigned int m_rows, m_columns, m_height;
}; // class

} // namespace mazes

#endif // GRID_H
