#include <MazeBuilder/lab.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/enums.h>

#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

using namespace mazes;

/// @brief Create a bidirectional or unidirectional link between two cells
/// @param c1 First cell to link
/// @param c2 Second cell to link  
/// @param bidi If true, creates bidirectional link; if false, only c1 links to c2
void lab::link(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, bool bidi) noexcept {
    if (!c1 || !c2) {

        return;
    }

    // Add c2 to c1's links
    c1->add_link(c2);

    // If bidirectional, add c1 to c2's links
    if (bidi) {

        c2->add_link(c1);
    }
}

/// @brief Remove link between two cells
/// @param c1 First cell to unlink
/// @param c2 Second cell to unlink
/// @param bidi If true, removes bidirectional link; if false, only removes c1's link to c2
void lab::unlink(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, bool bidi) noexcept {
    if (!c1 || !c2) {

        return;
    }

    // Remove c2 from c1's links
    c1->remove_link(c2);

    // If bidirectional, remove c1 from c2's links
    if (bidi) {

        c2->remove_link(c1);
    }
}

/// @brief Sets up neighbor relationships for a collection of cells based on grid topology
/// @param config Configuration containing program arguments
/// @param indices Vector of indices for reordering cells (optional - can be empty)
/// @param cells_to_set Output vector that will be populated with created and linked cells
/// 
/// @details This function performs the following operations:
/// 1. Validates input parameters to prevent overflow and invalid states
/// 2. Creates a grid of cells based on rows x columns x levels
/// 3. Optionally reorders cells based on provided indices 
/// 4. Establishes neighbor relationships in 4 cardinal directions
/// 5. Links cells to their neighbors for maze traversal
/// 
/// @warning This function can consume significant memory for large grids
/// @warning Infinite loops can occur if indices contain invalid values
/// 
/// Memory Complexity: O(rows * columns * levels * neighbors_per_cell)
/// Time Complexity: O(rows * columns * levels)
void lab::set_neighbors(configurator const& config, const std::vector<int>& indices, std::vector<std::shared_ptr<cell>>& cells_to_set) noexcept {

    using namespace std;

    // Check for uninitialized or corrupted configurator first
    if (!config.is_valid()) {
#if defined(MAZE_DEBUG)

        cerr << "Possible uninitialized configurator." << endl;
#endif

        cells_to_set.clear();

        return;
    }

    // Ensure we have valid configuration
    const auto rows = config.rows();
    const auto columns = config.columns();
    const auto levels = config.levels();

    /// @brief Lambda to calculate linear cell index from 3D coordinates
    /// @param row Row coordinate (0-based)
    /// @param col Column coordinate (0-based) 
    /// @param level Level coordinate (0-based)
    /// @return Linear index for accessing 1D array representation
    auto calculate_cell_index = [rows, columns](size_t row, size_t col, size_t level) -> size_t {

        return level * (rows * columns) + row * columns + col;
    };

    // Calculate total cells with already validated parameters
    const size_t total_cells = static_cast<size_t>(rows) * static_cast<size_t>(columns) * static_cast<size_t>(levels);

    try {

        cells_to_set.clear();

        cells_to_set.reserve(total_cells);
        
        // Create cells with sequential indices
        for (size_t index = 0; index < total_cells; ++index) {

            // Ensure index fits in int32_t for cell constructor
            if (index > static_cast<size_t>(numeric_limits<int32_t>::max())) {

                throw runtime_error("Index exceeds int32_t maximum");
            }
            
            cells_to_set.emplace_back(make_shared<cell>(static_cast<int32_t>(index)));
        }
    } catch (const exception& ex) {
        // Handle memory allocation failure gracefully
        cells_to_set.clear();

        cerr << "Memory allocation failed: " << ex.what() << endl;

        return;
    }

    /// @brief Reorder cells based on provided indices if valid
    /// @details This allows for shuffled maze generation by permuting cell order
    if (!indices.empty()) {
        
        try {

            if (total_cells != indices.size()) {

                throw runtime_error("Indices count mismatch: expected " + to_string(total_cells) + ", got " + to_string(indices.size()));
            }


            vector<shared_ptr<cell>> reordered_cells;
            reordered_cells.reserve(total_cells);
                
            // Validate all indices before proceeding to prevent infinite loops
            for (size_t i = 0; i < indices.size(); ++i) {

                const int index = indices.at(i);
                    
                // Check bounds and prevent potential infinite loop conditions
                if (index < 0 || static_cast<size_t>(index) >= cells_to_set.size()) {

                    throw runtime_error("Invalid index at position " + to_string(i) + ": " + to_string(index) + " (valid range: 0-" + to_string(cells_to_set.size() - 1) + ")");
                }
                    
                // Add the reordered cell
                reordered_cells.push_back(cells_to_set.at(static_cast<size_t>(index)));
            }
                
            // Only apply reordering if all indices were valid
            if (reordered_cells.size() == cells_to_set.size()) {

                cells_to_set = std::move(reordered_cells);
            }
        } catch (const exception&) {

            // Continue with original order if reordering fails
        }
    } // !indices.empty()

    /// @brief Create neighbor topology mapping for efficient neighbor lookup
    /// @details Maps each cell index to its neighbors in 4 cardinal directions
    unordered_map<size_t, unordered_map<Direction, size_t>> topology;

    try {
        // Reserve space for topology to prevent frequent reallocations
        topology.reserve(total_cells);
        
        // Triple nested loop - potential infinite loop source if parameters are invalid
        // Using explicit bounds checking to prevent infinite loops
        for (size_t l = 0; l < levels; ++l) {
            
            // Safety check to prevent infinite loop if levels was modified during execution
            if (l >= static_cast<size_t>(config.levels())) {

                throw runtime_error("Level bounds exceeded during iteration");
            }
            
            for (size_t r = 0; r < rows; ++r) {
                
                // Safety check to prevent infinite loop if rows was modified
                if (r >= static_cast<size_t>(config.rows())) {

                    throw runtime_error("Row bounds exceeded during iteration");
                }
                
                for (size_t c = 0; c < columns; ++c) {
                    
                    // Safety check to prevent infinite loop if columns was modified  
                    if (c >= static_cast<size_t>(config.columns())) {

                        throw runtime_error("Column bounds exceeded during iteration");
                    }
                    
                    // Calculate current cell index
                    size_t cell_index = calculate_cell_index(r, c, l);
                    
                    // Additional bounds check for calculated index
                    if (cell_index >= total_cells) {

                        throw runtime_error("Calculated index " + to_string(cell_index) + " exceeds total_cells " + to_string(total_cells));
                    }
                    
                    // Create neighbor mapping for current cell
                    unordered_map<Direction, size_t> neighbors;

                    // North neighbor (row - 1)
                    if (r > 0) {

                        size_t north_index = calculate_cell_index(r - 1, c, l);
                        
                        if (north_index < total_cells) {

                            neighbors[Direction::NORTH] = north_index;
                        }
                    }

                    // South neighbor (row + 1) 
                    if (r < rows - 1) {

                        size_t south_index = calculate_cell_index(r + 1, c, l);

                        if (south_index < total_cells) {

                            neighbors[Direction::SOUTH] = south_index;
                        }
                    }

                    // East neighbor (column + 1)
                    if (c < columns - 1) {

                        size_t east_index = calculate_cell_index(r, c + 1, l);

                        if (east_index < total_cells) {

                            neighbors[Direction::EAST] = east_index;
                        }
                    }

                    // West neighbor (column - 1)
                    if (c > 0) {

                        size_t west_index = calculate_cell_index(r, c - 1, l);

                        if (west_index < total_cells) {

                            neighbors[Direction::WEST] = west_index;
                        }
                    }

                    topology[cell_index] = neighbors;
                }
            }
        }
    } catch (const exception& ex) {

        cerr << "Topology creation failed: " << ex.what() << endl;

        return;
    }

    /// @brief Establish actual neighbor links between cells
    /// @details Iterates through all cells and links them to their neighbors    
    try {

        // Iterate through all created cells with bounds checking
        for (auto&& c : cells_to_set) {
            
            // Skip null cells to prevent crashes
            if (!c) {

                continue;
            }
            
            // Find topology entry for this cell
            auto topology_it = topology.find(c->get_index());

            if (topology_it == topology.end()) {

                // No neighbors for this cell - this is normal for boundary cells
                continue;
            }
            
            const auto& neighbors = topology_it->second;
            
            // Link to each neighbor with bounds checking
            for (const auto& [direction, neighbor_index] : neighbors) {

                // Verify neighbor index is within valid range
                if (neighbor_index >= cells_to_set.size()) {

                    throw runtime_error("Neighbor index " + to_string(neighbor_index) + " out of bounds (max: " + to_string(cells_to_set.size() - 1) + ")");
                }

                auto& neighbor_cell = cells_to_set.at(neighbor_index);
                    
                // Verify neighbor cell exists before linking
                if (!neighbor_cell) {

                    throw runtime_error("Null neighbor cell at index " + to_string(neighbor_index));
                }

                link(cref(c), cref(neighbor_cell), true);
            }
        }
    } catch (const exception& ex) {

        cerr << "Neighbor linking failed: " << ex.what() << endl;

        return;
    }
}
