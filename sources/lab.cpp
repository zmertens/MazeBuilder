#include <MazeBuilder/lab.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/string_view_utils.h>

#include <random>
#include <sstream>
#include <vector>

#if defined(MAZE_DEBUG)
#include <iostream>
#endif

using namespace mazes;

/// @brief Default constructor - initializes empty lab with default values
lab::lab()
: m_p_q{}
, levels{} {

}

lab::~lab() = default;

/// @brief Copy constructor - performs deep copy of lab state
/// @param other The lab object to copy from
lab::lab(const lab& other)
: m_p_q(other.m_p_q)
, levels{} {

}

/// @brief Copy assignment operator with self-assignment protection
/// @param other The lab object to copy from
/// @return Reference to this lab object
lab& lab::operator=(const lab& other) {
    if (this == &other) {
        return *this;
    }
    m_p_q = other.m_p_q;
    return *this;
}

/// @brief Find an entry in the lab by coordinates
/// @param p First coordinate
/// @param q Second coordinate  
/// @return Optional tuple containing the found entry, or nullopt if not found
std::optional<std::tuple<int, int, int, int>> lab::find(int p, int q) const noexcept {
    auto itr = m_p_q.find({ p, q });
    return (itr != m_p_q.cend()) ? make_optional(itr->second) : std::nullopt;
}

/// @brief Insert or update an entry in the lab
/// @param p First coordinate
/// @param q Second coordinate
/// @param r Third coordinate  
/// @param w Fourth coordinate
void lab::insert(int p, int q, int r, int w) noexcept {
    m_p_q.insert_or_assign({ p, q }, std::make_tuple(p, q, r, w));
}

/// @brief Check if the lab is empty
/// @return True if no entries exist, false otherwise
bool lab::empty() const noexcept {
    return m_p_q.empty();
}

/// @brief Get the number of levels in the lab
/// @return Current number of levels
int lab::get_levels() const noexcept {
    return levels;
}

/// @brief Set the number of levels in the lab
/// @param levels New number of levels to set
void lab::set_levels(int levels) noexcept {
    this->levels = levels;
}

/// @brief Generate a random block ID using fixed seed for reproducibility
/// @return Random block ID between 0 and 23
int lab::get_random_block_id() const noexcept {
    using namespace std;

    mt19937 mt{ 42681ul };
    auto get_int = [&mt](int low, int high) {
        uniform_int_distribution<int> dist{ low, high };
        return dist(mt);
        };

    return get_int(0, 23);
}

/// @brief Create a bidirectional or unidirectional link between two cells
/// @param c1 First cell to link
/// @param c2 Second cell to link  
/// @param bidi If true, creates bidirectional link; if false, only c1 links to c2
void lab::link(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, bool bidi) noexcept {
    if (!c1 || !c2) return;

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
    if (!c1 || !c2) return;

    // Remove c2 from c1's links
    c1->remove_link(c2);

    // If bidirectional, remove c1 from c2's links
    if (bidi) {
        c2->remove_link(c1);
    }
}

/// @brief Sets up neighbor relationships for a collection of cells based on grid topology
/// @param config Configuration containing maze dimensions and parameters
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

    // PHASE 0: CONFIGURATION CORRUPTION DETECTION  
    // Check for uninitialized or corrupted configurator first to prevent infinite loops
    if (!config.is_valid()) {
#if defined(MAZE_DEBUG)
        std::cerr << "lab::set_neighbors - Configuration validation failed! Possible uninitialized configurator." << std::endl;
        std::cerr << "lab::set_neighbors - Raw values: rows=" << config.rows() 
                  << ", columns=" << config.columns() << ", levels=" << config.levels() << std::endl;
#endif
        // Clear output and return early to prevent infinite loops/crashes
        cells_to_set.clear();
        return;
    }

    // PHASE 1: PARAMETER VALIDATION
    // Ensure we have valid configuration to prevent infinite loops and memory issues
    const auto rows = config.rows();
    const auto columns = config.columns();
    const auto levels = config.levels();
    
#if defined(MAZE_DEBUG)
    std::cerr << "lab::set_neighbors - Input validation: rows=" << rows 
              << ", columns=" << columns << ", levels=" << levels 
              << ", indices.size()=" << indices.size() << std::endl;
#endif
    
    // Double-check individual values even after config.is_valid()
    // This catches edge cases where config might have been corrupted after validation
    if (rows == 0 || columns == 0 || levels == 0) {
#if defined(MAZE_DEBUG)
        std::cerr << "lab::set_neighbors - Invalid dimensions detected, returning early" << std::endl;
#endif
        cells_to_set.clear();
        return;
    }
    
    // Additional check for obviously corrupted values (common corruption patterns)
    const unsigned int CORRUPTION_THRESHOLD = 0xFFFF0000; // High bit patterns often indicate corruption
    if (rows > CORRUPTION_THRESHOLD || columns > CORRUPTION_THRESHOLD || levels > CORRUPTION_THRESHOLD) {
#if defined(MAZE_DEBUG)
        std::cerr << "lab::set_neighbors - Detected likely memory corruption in configuration values" << std::endl;
        std::cerr << "lab::set_neighbors - Suspicious values: rows=0x" << std::hex << rows 
                  << ", columns=0x" << columns << ", levels=0x" << levels << std::dec << std::endl;
#endif
        cells_to_set.clear();
        return;
    }
    
    // PHASE 2: OVERFLOW PROTECTION
    // Prevent integer overflow that could cause infinite loops or bad_alloc
    // Check each dimension against reasonable limits
    const size_t MAX_DIMENSION = 10000;  // Reasonable upper limit
    if (rows > MAX_DIMENSION || columns > MAX_DIMENSION || levels > MAX_DIMENSION) {
#if defined(MAZE_DEBUG)
        std::cerr << "lab::set_neighbors - Dimensions exceed maximum allowed size: " 
                  << "rows=" << rows << ", columns=" << columns << ", levels=" << levels << std::endl;
#endif
        cells_to_set.clear();
        return;
    }
    
    // Check for potential overflow in multiplication before calculating total_cells
    // Using division to check: if a * b > max, then a > max / b
    const size_t max_size_t = std::numeric_limits<size_t>::max();
    if (rows > max_size_t / columns || 
        (rows * columns) > max_size_t / levels) {
#if defined(MAZE_DEBUG)
        std::cerr << "lab::set_neighbors - Potential overflow in total_cells calculation" << std::endl;
#endif
        cells_to_set.clear();
        return;
    }
    
    // Additional memory allocation limit check
    const size_t max_memory_elements = max_size_t / (sizeof(std::shared_ptr<cell>) * 2);
    if (rows > max_memory_elements / columns / levels) {
#if defined(MAZE_DEBUG)
        std::cerr << "lab::set_neighbors - Would exceed memory allocation limits" << std::endl;
#endif
        cells_to_set.clear();
        return;
    }

    // PHASE 3: SAFE SIZE CALCULATION
    /// @brief Lambda to calculate linear cell index from 3D coordinates
    /// @param row Row coordinate (0-based)
    /// @param col Column coordinate (0-based) 
    /// @param level Level coordinate (0-based)
    /// @return Linear index for accessing 1D array representation
    auto calculate_cell_index = [rows, columns](size_t row, size_t col, size_t level) -> size_t {
        // Formula: index = level * (rows * columns) + row * columns + col
        // This maps 3D coordinates to 1D array index
        return level * (rows * columns) + row * columns + col;
    };

    // Calculate total cells with already validated parameters
    const size_t total_cells = static_cast<size_t>(rows) * static_cast<size_t>(columns) * static_cast<size_t>(levels);
    
#if defined(MAZE_DEBUG)
    std::cerr << "lab::set_neighbors - Creating " << total_cells << " cells" << std::endl;
#endif

    // PHASE 4: CELL CREATION WITH EXCEPTION SAFETY
    try {
        // Clear existing cells and reserve space to prevent reallocations
        cells_to_set.clear();
        cells_to_set.reserve(total_cells);
        
        // Create cells with sequential indices
        for (size_t index = 0; index < total_cells; ++index) {
            // Ensure index fits in int32_t for cell constructor
            if (index > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
#if defined(MAZE_DEBUG)
                std::cerr << "lab::set_neighbors - Index exceeds int32_t maximum" << std::endl;
#endif
                cells_to_set.clear();
                return;
            }
            
            cells_to_set.emplace_back(std::make_shared<cell>(static_cast<int32_t>(index)));
        }
    } catch (const std::exception& ex) {
        // Handle memory allocation failure gracefully
        cells_to_set.clear();

#if defined(MAZE_DEBUG)
        std::cerr << "lab::set_neighbors - Memory allocation failed: " << ex.what() << std::endl;
#endif
        return;
    }

    // PHASE 5: OPTIONAL CELL REORDERING WITH VALIDATION
    /// @brief Reorder cells based on provided indices if valid
    /// @details This allows for shuffled maze generation by permuting cell order
    if (!indices.empty()) {
#if defined(MAZE_DEBUG)
        std::cerr << "lab::set_neighbors - Reordering cells with " << indices.size() << " indices" << std::endl;
#endif
        
        // Only reorder if indices count matches cell count exactly
        if (indices.size() == total_cells) {
            try {
                std::vector<shared_ptr<cell>> reordered_cells;
                reordered_cells.reserve(total_cells);
                
                // Validate all indices before proceeding to prevent infinite loops
                bool valid_indices = true;
                for (size_t i = 0; i < indices.size() && valid_indices; ++i) {
                    const int index = indices[i];
                    
                    // Check bounds and prevent potential infinite loop conditions
                    if (index < 0 || static_cast<size_t>(index) >= cells_to_set.size()) {
                        valid_indices = false;
#if defined(MAZE_DEBUG)
                        std::cerr << "lab::set_neighbors - Invalid index at position " << i 
                                  << ": " << index << " (valid range: 0-" << (cells_to_set.size() - 1) << ")" << std::endl;
#endif
                        break;
                    }
                    
                    // Add the reordered cell
                    reordered_cells.push_back(cells_to_set[static_cast<size_t>(index)]);
                }
                
                // Only apply reordering if all indices were valid
                if (valid_indices && reordered_cells.size() == cells_to_set.size()) {
                    cells_to_set = std::move(reordered_cells);
#if defined(MAZE_DEBUG)
                    std::cerr << "lab::set_neighbors - Successfully reordered cells" << std::endl;
#endif
                }
            } catch (const std::exception& ex) {
#if defined(MAZE_DEBUG)
                std::cerr << "lab::set_neighbors - Reordering failed: " << ex.what() << std::endl;
#endif
                // Continue with original order if reordering fails
            }
        } else {
#if defined(MAZE_DEBUG)
            std::cerr << "lab::set_neighbors - Indices count mismatch: expected " 
                      << total_cells << ", got " << indices.size() << std::endl;
#endif
        }
    }

    // PHASE 6: TOPOLOGY CREATION WITH SAFE ITERATION
    /// @brief Create neighbor topology mapping for efficient neighbor lookup
    /// @details Maps each cell index to its neighbors in 4 cardinal directions
    std::unordered_map<size_t, std::unordered_map<Direction, size_t>> topology;
    
    try {
        // Reserve space for topology to prevent frequent reallocations
        topology.reserve(total_cells);
        
#if defined(MAZE_DEBUG)
        std::cerr << "lab::set_neighbors - Building topology for " << levels << " levels" << std::endl;
#endif
        
        // Triple nested loop - potential infinite loop source if parameters are invalid
        // Using explicit bounds checking to prevent infinite loops
        for (size_t l = 0; l < levels; ++l) {
            
            // Safety check to prevent infinite loop if levels was modified during execution
            if (l >= static_cast<size_t>(config.levels())) {
#if defined(MAZE_DEBUG)
                std::cerr << "lab::set_neighbors - Level bounds exceeded during iteration" << std::endl;
#endif
                break;
            }
            
            for (size_t r = 0; r < rows; ++r) {
                
                // Safety check to prevent infinite loop if rows was modified
                if (r >= static_cast<size_t>(config.rows())) {
#if defined(MAZE_DEBUG)
                    std::cerr << "lab::set_neighbors - Row bounds exceeded during iteration" << std::endl;
#endif
                    break;
                }
                
                for (size_t c = 0; c < columns; ++c) {
                    
                    // Safety check to prevent infinite loop if columns was modified  
                    if (c >= static_cast<size_t>(config.columns())) {
#if defined(MAZE_DEBUG)
                        std::cerr << "lab::set_neighbors - Column bounds exceeded during iteration" << std::endl;
#endif
                        break;
                    }
                    
                    // Calculate current cell index
                    size_t cell_index = calculate_cell_index(r, c, l);
                    
                    // Additional bounds check for calculated index
                    if (cell_index >= total_cells) {
#if defined(MAZE_DEBUG)
                        std::cerr << "lab::set_neighbors - Calculated index " << cell_index 
                                  << " exceeds total_cells " << total_cells << std::endl;
#endif
                        continue;
                    }
                    
                    // Create neighbor mapping for current cell
                    std::unordered_map<Direction, size_t> neighbors;

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
    } catch (const std::exception& ex) {
#if defined(MAZE_DEBUG)
        std::cerr << "lab::set_neighbors - Topology creation failed: " << ex.what() << std::endl;
#endif
        return;
    }

    // PHASE 7: NEIGHBOR RELATIONSHIP ESTABLISHMENT
    /// @brief Establish actual neighbor links between cells
    /// @details Iterates through all cells and links them to their neighbors
#if defined(MAZE_DEBUG)
    std::cerr << "lab::set_neighbors - Establishing neighbor relationships" << std::endl;
#endif
    
    try {
        // Iterate through all created cells with bounds checking
        for (size_t i = 0; i < cells_to_set.size(); ++i) {
            auto& c = cells_to_set[i];
            
            // Skip null cells to prevent crashes
            if (!c) {
#if defined(MAZE_DEBUG)
                std::cerr << "lab::set_neighbors - Null cell at index " << i << std::endl;
#endif
                continue;
            }
            
            // Find topology entry for this cell
            auto topology_it = topology.find(i);
            if (topology_it == topology.end()) {
                // No neighbors for this cell - this is normal for boundary cells
                continue;
            }
            
            const auto& neighbors = topology_it->second;
            
            // Link to each neighbor with bounds checking
            for (const auto& [direction, neighbor_index] : neighbors) {
                // Verify neighbor index is within valid range
                if (neighbor_index < cells_to_set.size()) {
                    auto& neighbor_cell = cells_to_set[neighbor_index];
                    
                    // Verify neighbor cell exists before linking
                    if (neighbor_cell) {
                        c->add_link(neighbor_cell);
                    }
#if defined(MAZE_DEBUG)
                    else {
                        std::cerr << "lab::set_neighbors - Null neighbor cell at index " 
                                  << neighbor_index << std::endl;
                    }
#endif
                }
#if defined(MAZE_DEBUG)
                else {
                    std::cerr << "lab::set_neighbors - Neighbor index " << neighbor_index 
                              << " out of bounds (max: " << (cells_to_set.size() - 1) << ")" << std::endl;
                }
#endif
            }
        }
    } catch (const std::exception& ex) {
#if defined(MAZE_DEBUG)
        std::cerr << "lab::set_neighbors - Neighbor linking failed: " << ex.what() << std::endl;
#endif
        // Continue execution - partial linking is better than total failure
    }

#if defined(MAZE_DEBUG)
    std::cerr << "lab::set_neighbors - Completed successfully with " 
              << cells_to_set.size() << " cells" << std::endl;
#endif
}
