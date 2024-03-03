#ifndef GRID_H
#define GRID_H

#include <vector>
#include <memory>
#include <sstream>
#include <string>

#include "cell.h"

namespace mazes {

class grid {
public:
    grid(unsigned int rows, unsigned int columns);
    std::vector<std::vector<std::shared_ptr<cell>>> get_grid() const;
    unsigned int get_rows() const noexcept;
    unsigned int get_columns() const noexcept;
private:
    void prepare_grid(std::vector<std::vector<std::shared_ptr<cell>>>& grid) noexcept;
    void configure_cells(std::vector<std::vector<std::shared_ptr<cell>>>& grid) noexcept;
    void print_grid_cells(std::vector<std::vector<std::shared_ptr<cell>>> const& grid) const noexcept;

    friend std::ostream& operator<<(std::ostream& os, grid& g) {
        std::stringstream output;
        output << "+";
        for (auto i {0}; i < g.m_grid.size(); i++)
            output << "---+";
        output << "\n";
        for (auto row {0}; row < g.m_grid.size(); row++) {
            std::stringstream top_builder, bottom_builder;
            top_builder << "|";
            bottom_builder << "+";
            for (auto col {0}; col < g.m_grid.at(row).size(); col++) {
                auto&& temp = g.m_grid.at(row).at(col);
                if (temp == nullptr)
                    temp = std::make_shared<cell>(-1, -1);
                // 3 spaces in body
                std::string body = "   ";
                std::string east_boundary = temp->is_linked(temp->get_east()) ? " " : "|";
                top_builder << body << east_boundary;
                std::string south_boundary = temp->is_linked(temp->get_south()) ? "   " : "---";
                bottom_builder << south_boundary << "+";
            }
            output << top_builder.str() << "\n" << bottom_builder.str() << "\n";
            top_builder.clear();
            bottom_builder.clear();
        }

        os << output.str() << std::endl;

        return os;
    }

    const unsigned int m_rows, m_columns;
    std::vector<std::vector<std::shared_ptr<cell>>> m_grid;
};
} // namespace mazes
#endif // GRID_H
