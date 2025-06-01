#ifndef GRID_INTERFACE_H
#define GRID_INTERFACE_H

#include <vector>
#include <string>
#include <ostream>
#include <sstream>
#include <memory>
#include <cstdint>
#include <optional>
#include <tuple>
#include <functional>

#include <MazeBuilder/enums.h>
#include <MazeBuilder/cell.h>

namespace mazes {

/// @file grid_interface.h
/// @class grid_interface
/// @brief Interface for the grid class
/// @details This interface provides methods to interact with the grid
/// @details The grid is configurable
/// @details The grid can be visualized as a 2D or 3D structure using strings
/// @details The grid can be transformed into a vector of cells or a vector of vectors of cells
/// @details The grid can provide detailed information about a cell
class grid_interface {

public:
    virtual ~grid_interface() = default;

    /// @brief Get dimensions of a grid with no assumptions about the ordering of the dimensions
    virtual std::tuple<unsigned int, unsigned int, unsigned int> get_dimensions() const noexcept = 0;

    virtual std::shared_ptr<cell> get_east(std::shared_ptr<cell> const& c) const noexcept = 0;

    virtual std::shared_ptr<cell> get_south(std::shared_ptr<cell> const& c) const noexcept = 0;

    /// @brief Transformation and display functions
    virtual void to_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept = 0;	

    /// @brief Get detailed information of a cell in the grid
    /// @param c 
    /// @return 
    virtual std::optional<std::string> contents_of(const std::shared_ptr<cell>& c) const noexcept = 0;

    /// @brief Returns the background color for the specified cell, if available.
    /// @param c A shared pointer to the cell for which to determine the background color.
    /// @return An optional 32-bit unsigned integer representing the background color of the cell, or std::nullopt if no color is specified.
    virtual std::optional<std::uint32_t> background_color_for(const std::shared_ptr<cell>& c) const noexcept = 0;

protected:

    /// @brief Provide a string representation of the grid
    /// @param os 
    /// @param g 
    /// @return 
    friend std::ostream& operator<<(std::ostream& os, const grid_interface& g) {
        auto [rows, columns, _] = g.get_dimensions();

        // First sort cells by row then column
        std::vector<std::shared_ptr<cell>> cells;
        cells.reserve(rows * columns);

        // populate the cells from the grid
        g.to_vec(std::ref(cells));

        // ---+
        static constexpr auto barrier = { BARRIER2, BARRIER2, BARRIER2, BARRIER2, BARRIER2, CORNER };
        static const std::string wall_plus_corner = "-----+";

        std::stringstream output;
        output << CORNER;

        for (auto i{ 0u }; i < columns; i++) {
            output << wall_plus_corner;
        }
        output << "\n";

        auto row_counter{ 0u }, column_counter{ 0u };
        auto cell_iter = cells.cbegin();

        while (row_counter < rows) {
            std::stringstream top_builder, bottom_builder;
            top_builder << BARRIER1;
            bottom_builder << CORNER;

            while (column_counter < columns && cell_iter != cells.cend()) {

                // 5 spaces in body for single-digit number to hold base36 values
                static const std::string vertical_barrier_str{ BARRIER1 };

                auto val = g.contents_of(std::cref(*cell_iter)).value_or("*");
                std::string body = "";
                switch (val.size()) {
                case 1: body = "  " + val + "  "; break;
                case 2: body = " " + val + "  "; break;
                case 3: body = " " + val + " "; break;
                case 4: body = " " + val; break;
                    // case 1 is default
                default: body = "  " + val + "  "; break;
                }

                // Use the grid to get neighbors instead of calling methods on the cell directly
                auto current_cell = *cell_iter;
                auto east_neighbor = g.get_east(current_cell);
                auto south_neighbor = g.get_south(current_cell);

                bool has_east_link = east_neighbor && current_cell->is_linked(east_neighbor);
                bool has_south_link = south_neighbor && current_cell->is_linked(south_neighbor);

                auto east_boundary = has_east_link ? " " : vertical_barrier_str;
                auto south_boundary = has_south_link ? "     " : wall_plus_corner.substr(0, wall_plus_corner.size() - 1);

                top_builder << body << east_boundary;
                bottom_builder << south_boundary << "+";

                ++cell_iter;
                ++column_counter;
            }

            column_counter = 0;
            ++row_counter;

            output << top_builder.str() << "\n" << bottom_builder.str() << "\n";
        } // while

        os << output.str() << "\n";

        return os;
    } // << operator

}; // grid_interface

} // namespace mazes

#endif // GRID_INTERFACE_H
