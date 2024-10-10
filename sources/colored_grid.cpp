#include "colored_grid.h"

#include <iostream>
#include <optional>

#include "distance_grid.h"
#include "distances.h"
#include "cell.h"
#include "grid.h"

using namespace mazes;
using namespace std;

/**
 * @brief Construct a new distance grid object
 * @param width
 * @param length
 * @param height 0
 */
colored_grid::colored_grid(unsigned int width, unsigned int length, unsigned int height)
    : grid(width, length, height) {

}

unsigned int colored_grid::get_rows() const noexcept {
    return grid::get_rows();
}

unsigned int colored_grid::get_columns() const noexcept {
	return grid::get_columns();
}

/**
 *
 * @param cell_size 3
 */
std::vector<std::uint8_t> colored_grid::to_pixels(const unsigned int cell_size) const noexcept {
    return grid::to_pixels(cell_size);
}

void colored_grid::make_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept {
    return grid::make_vec(ref(cells));
}

void colored_grid::append(std::unique_ptr<grid_interface> const& other_grid) noexcept {
	grid::append(other_grid);
}
void colored_grid::insert(std::shared_ptr<cell> const& parent, int index) noexcept {
	grid::insert(parent, index);
}

bool colored_grid::update(std::shared_ptr<cell>& parent, int old_index, int new_index) noexcept {
	return grid::update(ref(parent), old_index, new_index);
}

std::shared_ptr<cell> colored_grid::search(std::shared_ptr<cell> const& start, int index) const noexcept {
	return grid::search(start, index);
}
void colored_grid::del(std::shared_ptr<cell> parent, int index) noexcept {
	grid::del(parent, index);
}

std::shared_ptr<cell> colored_grid::get_root() const noexcept {
	return grid::get_root();
}

std::optional<std::string> colored_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
	return grid::contents_of(cref(c));
}

optional<uint32_t> colored_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
	const auto& dists = c->get_distances_from(this->get_root());
	if (!dists) {
		return nullopt;
	}

	auto max = dists->max();
	//auto max = 10;

	int distance1 = dists->operator[](c);
	//int distance1 = 5;
	float intensity = static_cast<float>(10 - distance1) / 10;
	int dark = static_cast<int>(255 * intensity);
	int bright = 128 + static_cast<int>(127 * intensity);

	return (dark << 16) | (bright << 8) | dark;
	//return grid::background_color_for(c);
}
