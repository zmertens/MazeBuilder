#include <MazeBuilder/distance_grid.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_operations.h>

#include <algorithm>
#include <numeric>
#include <stdexcept>

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

/// @brief Constructs a distance_grid object with specified dimensions and initializes the distance calculations.
/// @return future to init task
//void distance_grid::configure(const std::vector<int>& indices) noexcept {
//
//    grid::configure(cref(indices));
//}

std::string distance_grid::contents_of(std::shared_ptr<cell> const& c) const noexcept {
    if (m_distances && c) {

        // Check if the cell exists in our distance map
        if (m_distances->contains(c->get_index())) {

            const auto d = m_distances->operator[](c->get_index());
            if (d >= 0) {

                // Convert distance to base36 representation for more compact display
                return to_base36(d);
            }
        }
    }

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

//    try {
//        auto start_cell = m_grid->search(start_index);
//        auto end_cell = m_grid->search(end_index);
//
//        if (!start_cell || !end_cell) {
//
//            throw std::runtime_error("Invalid start or end cell index.");
//        }
//
//        m_distances = std::make_shared<distances>(start_cell->get_index());
//
//        if (!m_distances) {
//
//            throw std::runtime_error("Failed to create distances object.");
//        }
//
//        m_distances = m_distances->path_to(end_cell->get_index(), *this);
//
//        if (!m_distances) {
//
//            throw std::runtime_error("Failed to get path to goal.");
//        }
//
//    } catch (const std::exception& e) {
//
//#if defined(MAZE_DEBUG)
//        std::cerr << "Exception in distance_grid::calculate_distances: " << e.what() << std::endl;
//#endif
    //}
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

