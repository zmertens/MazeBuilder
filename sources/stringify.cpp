#include <MazeBuilder/stringify.h>

#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/maze_adapter.h>
#include <MazeBuilder/randomizer.h>

#include <sstream>
#include <iostream>

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

    // Safety check for excessively large grids to prevent performance issues
    const size_t total_cells = static_cast<size_t>(rows) * static_cast<size_t>(columns) * static_cast<size_t>(levels);
    const size_t max_reasonable_cells = 10000; // Reasonable limit for text output
    
    if (total_cells > max_reasonable_cells) {
#if defined(MAZE_DEBUG)
        std::cerr << "stringify::run - Grid too large (" << total_cells << " cells), max recommended: " << max_reasonable_cells << std::endl;
#endif
        return false;
    }

    // Get all cells using maze_adapter
    auto all_cells = ops.get_cells();
    
    if (all_cells.empty()) {
#if defined(MAZE_DEBUG)
        std::cerr << "stringify::run - No cells in grid" << std::endl;
#endif
        return false;
    }
    
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
            
            // SAFETY CHECK: Prevent infinite loop in content padding
            if (content.length() > 10) {
                content = content.substr(0, 3); // Truncate if too long
            }
            
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
                
                // Safety check: limit the number of links to check
                size_t link_count = 0;
                const size_t max_links = 100; // Reasonable upper bound
                
                for (const auto& [linked_cell, is_linked] : links) {
                    if (++link_count > max_links) {
#if defined(MAZE_DEBUG)
                        std::cerr << "stringify::run - Too many links in cell, breaking" << std::endl;
#endif
                        break;
                    }
                    
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
                
                // Safety check: limit the number of links to check
                size_t link_count = 0;
                const size_t max_links = 100; // Reasonable upper bound
                
                for (const auto& [linked_cell, is_linked] : links) {
                    if (++link_count > max_links) {
#if defined(MAZE_DEBUG)
                        std::cerr << "stringify::run - Too many links in cell, breaking" << std::endl;
#endif
                        break;
                    }
                    
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
