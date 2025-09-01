#include <MazeBuilder/sidewinder.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>

#include <functional>
#include <vector>

using namespace mazes;

/// @brief Generates a perfect maze if done correctly (no loops) by using "runs" to carve east-west
/// @details "Runs" are row-like passages that are carved by the sidewinder algorithm
/// @param g the grid to generate the maze on, and manipulate the cells
/// @param get_int
/// @param rng
bool sidewinder::run(grid_interface* g, randomizer& rng) const noexcept {

    using namespace std;

    if (!g) {

        return false;
    }

    const auto& grid_ops = g->operations();

    auto [rows, columns, _] = grid_ops.get_dimensions();

    // Iterator-based approach: process cells row by row without materializing all cells
    // This is much more memory efficient for large grids
    
    for (unsigned int row = 0; row < rows; ++row) {

        vector<shared_ptr<cell>> run;  // Current run of cells

        for (unsigned int col = 0; col < columns; ++col) {

            // Calculate cell index for current position
            int cell_index = static_cast<int>(row * columns + col);
            
            // Get cell (will be created lazily if it doesn't exist)
            auto cell = grid_ops.search(cell_index);
            if (!cell) continue;

            // Add current cell to the run
            run.push_back(cell);

            bool at_eastern_boundary = (col == columns - 1);
            bool at_northern_boundary = (row == 0);

            // Should we close out this run?
            // Either at eastern boundary or randomly decide to close
            bool should_close_out = at_eastern_boundary ||
                (!at_northern_boundary && rng(0, 1) == 0);

            if (should_close_out && !run.empty()) {
                // Select a random cell from the run to connect northward
                // (unless we're at the northern boundary)
                if (!at_northern_boundary) {
                    size_t random_index = rng(0, static_cast<int>(run.size()) - 1);
                    auto random_cell = run[random_index];

                    if (random_cell) {
                        auto north_cell = grid_ops.get_north(random_cell);
                        if (north_cell) {
                            lab::link(random_cell, north_cell, true);
                        }
                    }
                }

                // Clear the run to start a new one
                run.clear();
            } else if (!at_eastern_boundary) {
                // If not closing and not at eastern boundary, link east

                auto east_cell = grid_ops.get_east(cell);

                if (east_cell) {

                    lab::link(cell, east_cell, true);
                }
            }
        }
    }

    return true;
}
