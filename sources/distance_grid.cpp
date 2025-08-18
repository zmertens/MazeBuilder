#include <MazeBuilder/distance_grid.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_operations.h>

#include <algorithm>
#include <deque>
#include <numeric>
#include <stdexcept>
#include <unordered_map>

// Temporarily force debug output to help with debugging
#define MAZE_DEBUG 1

#if defined(MAZE_DEBUG)
#include <iostream>
#endif

using namespace mazes;

/// @brief Constructs a distance_grid object with specified dimensions and initializes the distance calculations.
/// @param rows 1
/// @param cols 1
/// @param levels 1
distance_grid::distance_grid(unsigned int rows, unsigned int cols, unsigned int levels)
    : m_grid{ std::make_unique<grid>(rows, cols, levels) } {

}

std::string distance_grid::contents_of(std::shared_ptr<cell> const& c) const noexcept {
#if defined(MAZE_DEBUG)
    static int call_count = 0;
    if (++call_count <= 5) {  // Only log first 5 calls to avoid spam
        std::cerr << "Debug: contents_of called for cell " << (c ? std::to_string(c->get_index()) : "null") << std::endl;
    }
#endif

    if (m_distances && c) {

        // Check if the cell exists in our distance map
        if (m_distances->contains(c->get_index())) {

            const auto d = m_distances->operator[](c->get_index());
            if (d >= 0) {

                // Convert distance to base36 representation for more compact display
                auto result = to_base36(d);
#if defined(MAZE_DEBUG)
                std::cerr << "Debug: Cell " << c->get_index() << " has distance " << d 
                         << " (base36: " << result << ")" << std::endl;
#endif
                return result;
            }
        }
#if defined(MAZE_DEBUG)
        else if (m_distances) {
            std::cerr << "Debug: Cell " << c->get_index() << " not found in distances map" << std::endl;
        }
#endif
    }
#if defined(MAZE_DEBUG)
    else {
        std::cerr << "Debug: m_distances is " << (m_distances ? "valid" : "null") 
                 << ", cell is " << (c ? "valid" : "null") << std::endl;
    }
#endif

    // Fall back to default representation if no distance info available
    return m_grid->contents_of(c);
}

std::uint32_t distance_grid::background_color_for(std::shared_ptr<cell> const& c) const noexcept {

	return m_grid->background_color_for(cref(c));
}

std::string distance_grid::to_base36(int value) const {

	static constexpr auto base36_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::string result;
	do {
		result.push_back(base36_chars[value % 36]);
		value /= 36;
	} while (value > 0);
	std::reverse(result.begin(), result.end());
	return result;
}

/// @brief 
/// @param start_index 
/// @param end_index 
void distance_grid::calculate_distances(int start_index, int end_index) noexcept {

#if defined(MAZE_DEBUG)
    std::cerr << "Debug: calculate_distances called with start=" << start_index 
             << ", end=" << end_index << std::endl;
#endif

   try {
        const auto& grid_ops = m_grid->operations();

        auto start_cell = grid_ops.search(start_index);
        if (!start_cell) {
#if defined(MAZE_DEBUG)
            std::cerr << "Debug: Invalid start cell at index " << start_index << std::endl;
#endif
            throw std::runtime_error("Invalid start cell index.");
        }

#if defined(MAZE_DEBUG)
        std::cerr << "Debug: Creating distances object with root=" << start_cell->get_index() << std::endl;
#endif

        // Create distances from start cell to all reachable cells
        m_distances = std::make_shared<distances>(start_cell->get_index());
        if (!m_distances) {
            throw std::runtime_error("Failed to create distances object.");
        }

        // Calculate distances from start cell to all reachable cells using BFS
        std::unordered_map<int32_t, bool> visited;
        std::deque<int32_t> queue;
        
        queue.push_back(start_index);
        visited[start_index] = true;
        m_distances->set(start_index, 0);

        while (!queue.empty()) {
            int32_t current_index = queue.front();
            queue.pop_front();
            
            auto current_cell = grid_ops.search(current_index);
            if (!current_cell) continue;
            
            int current_distance = (*m_distances)[current_index];
            
            // Get all neighbors
            auto neighbors = grid_ops.get_neighbors(current_cell);
            for (const auto& neighbor : neighbors) {
                if (!neighbor) continue;
                
                int32_t neighbor_index = neighbor->get_index();
                
                // Skip if already visited
                if (visited.find(neighbor_index) != visited.end()) {
                    continue;
                }
                
                // Only follow passages that exist (cells that are linked)
                if (!current_cell->is_linked(neighbor)) {
                    continue;
                }
                
                // Mark as visited and set distance
                visited[neighbor_index] = true;
                m_distances->set(neighbor_index, current_distance + 1);
                queue.push_back(neighbor_index);
            }
        }

#if defined(MAZE_DEBUG)
        std::cerr << "Debug: Calculated distances for " << visited.size() << " cells" << std::endl;
        
        // Log some distance values for debugging
        for (const auto& [cell_idx, _] : visited) {
            if (m_distances->contains(cell_idx)) {
                std::cerr << "Debug: Cell " << cell_idx << " has distance " << (*m_distances)[cell_idx] << std::endl;
            }
        }
#endif

   } catch (const std::exception& e) {

#if defined(MAZE_DEBUG)
       std::cerr << "Exception in calculating distances: " << e.what() << std::endl;
#endif
    }
}

std::shared_ptr<distances> distance_grid::get_distances() const noexcept {

	return this->m_distances;
}

// Delegate to embedded grid
grid_operations& distance_grid::operations() noexcept {

    return m_grid->operations();
}

const grid_operations& distance_grid::operations() const noexcept {

    return m_grid->operations();
}

