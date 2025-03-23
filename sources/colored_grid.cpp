#include <MazeBuilder/colored_grid.h>

#include <iostream>
#include <optional>

#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/cell.h>

using namespace mazes;
using namespace std;

/// @brief 
/// @param rows 1
/// @param cols 1
/// @param height 1 
colored_grid::colored_grid(unsigned int rows, unsigned int cols, unsigned int height)
    : distance_grid::grid(rows, cols, height)
	, m_distance_grid{ make_shared<distance_grid>(rows, cols, height) } {

}

/**
 * @brief Get the contents of a cell with color
 */
std::optional<std::string> colored_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
	return this->m_distance_grid->contents_of(cref(c));
}

optional<uint32_t> colored_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {	
	//const auto& dists = m_distance_grid->get_distances()->path_to(c);
	//if (!dists) {
		return nullopt;
	//}

	//auto max = dists->max();

	//int distance1 = dists->operator[](c);
	//float intensity = static_cast<float>(10 - distance1) / 10;
	//int dark = static_cast<int>(255 * intensity);
	//int bright = 128 + static_cast<int>(127 * intensity);
	//return (dark << 16) | (bright << 8) | dark;
	//return m_distance_grid->background_color_for(cref(c));
}
