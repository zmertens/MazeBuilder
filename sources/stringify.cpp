#include <MazeBuilder/stringify.h>

#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/randomizer.h>

using namespace mazes;

/// @brief Provide a string representation of the grid
/// @param g
/// @param rng
/// @return 
bool stringify::run(std::unique_ptr<grid_interface> const& g, randomizer& rng) const noexcept {

    //auto [rows, columns, _] = g.get_dimensions();

    //// First sort cells by row then column
    //std::vector<std::shared_ptr<cell>> cells;
    //cells.reserve(rows * columns);

    //// populate the cells from the grid
    //g.to_vec(std::ref(cells));

    //// ---+
    //static constexpr auto barrier = { BARRIER2, BARRIER2, BARRIER2, BARRIER2, BARRIER2, CORNER };
    //static const std::string wall_plus_corner = "-----+";

    //std::stringstream output;
    //output << CORNER;

    //for (auto i{ 0u }; i < columns; i++) {
    //    output << wall_plus_corner;
    //}
    //output << "\n";

    //auto row_counter{ 0u }, column_counter{ 0u };
    //auto cell_iter = cells.cbegin();

    //while (row_counter < rows) {
    //    std::stringstream top_builder, bottom_builder;
    //    top_builder << BARRIER1;
    //    bottom_builder << CORNER;

    //    while (column_counter < columns && cell_iter != cells.cend()) {

    //        // 5 spaces in body for single-digit number to hold base36 values
    //        static const std::string vertical_barrier_str{ BARRIER1 };

    //        auto val = g.contents_of(std::cref(*cell_iter));
    //        std::string body = "";
    //        switch (val.size()) {
    //        case 1: body = "  " + val + "  "; break;
    //        case 2: body = " " + val + "  "; break;
    //        case 3: body = " " + val + " "; break;
    //        case 4: body = " " + val; break;
    //            // case 1 is default
    //        default: body = "  " + val + "  "; break;
    //        }

    //        // Use the grid to get neighbors instead of calling methods on the cell directly
    //        auto current_cell = *cell_iter;
    //        auto east_neighbor = g.get_east(current_cell);
    //        auto south_neighbor = g.get_south(current_cell);

    //        bool has_east_link = east_neighbor && current_cell->is_linked(east_neighbor);
    //        bool has_south_link = south_neighbor && current_cell->is_linked(south_neighbor);

    //        auto east_boundary = has_east_link ? " " : vertical_barrier_str;
    //        auto south_boundary = has_south_link ? "     " : wall_plus_corner.substr(0, wall_plus_corner.size() - 1);

    //        top_builder << body << east_boundary;
    //        bottom_builder << south_boundary << "+";

    //        ++cell_iter;
    //        ++column_counter;
    //    }

    //    column_counter = 0;
    //    ++row_counter;

    //    output << top_builder.str() << "\n" << bottom_builder.str() << "\n";
    //} // while

    //os << output.str() << "\n";

    //return os;
    return false;
} // run
