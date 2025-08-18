#include <MazeBuilder/stringify.h>

#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/maze_adapter.h>
#include <MazeBuilder/randomizer.h>

#include <sstream>

using namespace mazes;

/// @brief Provide a string representation of the grid
/// @param g
/// @param rng
/// @return 
bool stringify::run(std::unique_ptr<grid_interface> const& g, [[maybe_unused]] randomizer& rng) const noexcept {

    if (!g) {
        return false;
    }

    std::stringstream ss;
    auto& ops = g->operations();

    auto [rows, columns, levels] = ops.get_dimensions();

    // Get all cells using maze_adapter
    auto all_cells = ops.get_cells();
    maze_adapter maze_view(all_cells);

    // Generate ASCII representation
    // Top border
    ss << "+";
    for (unsigned int c = 0; c < columns; ++c) {
        ss << "---+";
    }
    ss << "\n";

    // For each row
    for (unsigned int r = 0; r < rows; ++r) {
        std::string top_line = "|";
        std::string bottom_line = "+";

        // For each column
        for (unsigned int c = 0; c < columns; ++c) {
            int cell_index = r * columns + c;
            
            // Use maze_adapter to find the cell
            auto cell_it = maze_view.find(cell_index);
            auto cell_ptr = (cell_it != maze_view.end()) ? *cell_it : nullptr;

            // Cell content (from g's contents_of method)
            std::string content = cell_ptr ? g->contents_of(cell_ptr) : " ";
            // Pad content to 3 characters
            while (content.length() < 3) {
                content = " " + content;
            }

            top_line += content;

            // East wall
            auto east_neighbor = ops.get_east(cell_ptr);
            bool linked_east = false;

            if (cell_ptr && east_neighbor) {
                auto links = cell_ptr->get_links();
                for (const auto& [linked_cell, is_linked] : links) {
                    if (is_linked) {
                        auto locked = linked_cell;
                        if (locked && locked->get_index() == east_neighbor->get_index()) {
                            linked_east = true;
                            break;
                        }
                    }
                }
            }

            top_line += linked_east ? " " : "|";

            // South wall
            auto south_neighbor = ops.get_south(cell_ptr);
            bool linked_south = false;

            if (cell_ptr && south_neighbor) {
                auto links = cell_ptr->get_links();
                for (const auto& [linked_cell, is_linked] : links) {
                    if (is_linked) {
                        auto locked = linked_cell;
                        if (locked && locked->get_index() == south_neighbor->get_index()) {
                            linked_south = true;
                            break;
                        }
                    }
                }
            }

            bottom_line += linked_south ? "   " : "---";
            bottom_line += "+";
        }

        ss << top_line << "\n" << bottom_line << "\n";
    }

    ops.set_str(ss.str());

    return true;
} // run
