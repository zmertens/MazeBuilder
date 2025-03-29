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
    : distance_grid(rows, cols, height) {

}

std::optional<std::string> colored_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
	return distance_grid::contents_of(cref(c));
}

optional<uint32_t> colored_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
    if (!c) {

        return nullopt;
    }

	const auto& d = this->get_distances()->path_to(c);
	if (!d) {

        return distance_grid::background_color_for(cref(c));
	}

	auto max = d->max();

	int distance1 = d->operator[](c);

	float intensity = static_cast<float>(10 - distance1) / 10;
	int dark = static_cast<int>(255 * intensity);
	int bright = 128 + static_cast<int>(127 * intensity);
	return (dark << 16) | (bright << 8) | dark;
}
