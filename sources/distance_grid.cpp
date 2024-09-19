#include "distance_grid.h"

#include "distances.h"
#include "grid.h"
#include "cell.h"

using namespace mazes;
using namespace std;

/**
 * @brief Construct a new distance grid object
 * @param width
 * @param length
 * @param height 
 */
distance_grid::distance_grid(unsigned int width, unsigned int length, unsigned int height)
	: grid(width, length, height), m_distances(make_shared<distances>(this->get_root())) {
}

unsigned int distance_grid::get_rows() const noexcept {
	return this->grid::get_rows();
}

unsigned int distance_grid::get_columns() const noexcept {
	return this->grid::get_columns();
}

/**
 *
 * @param cell_size 3
 */
std::vector<std::uint8_t> distance_grid::to_pixels(const unsigned int cell_size) const noexcept {
	return this->grid::to_pixels(cell_size);
}

std::vector<std::shared_ptr<cell>> distance_grid::to_vec() const noexcept {
	return this->grid::to_vec();
}

void distance_grid::append(std::unique_ptr<grid_interface> const& other_grid) noexcept {
	this->grid::append(other_grid);
}
void distance_grid::insert(std::shared_ptr<cell> const& parent, int index) noexcept {
	this->grid::insert(parent, index);
}

bool distance_grid::update(std::shared_ptr<cell>& parent, int old_index, int new_index) noexcept {
	return this->grid::update(ref(parent), old_index, new_index);
}

std::shared_ptr<cell> distance_grid::search(std::shared_ptr<cell> const& start, int index) const noexcept {
	return this->grid::search(start, index);
}
void distance_grid::del(std::shared_ptr<cell> parent, int index) noexcept {
	this->grid::del(parent, index);
}

std::shared_ptr<cell> distance_grid::get_root() const noexcept {
	return grid::get_root();
}

std::optional<std::string> distance_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
	if (m_distances) {
		const auto d = m_distances->operator[](c);
		return d >= 0 ? to_base64(d) : grid::contents_of(c);
	}
	return grid::contents_of(c);
}

std::optional<std::uint32_t> distance_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
	return grid::background_color_for(cref(c));
}

optional<std::string> distance_grid::to_base64(int value) const {
	static const std::string base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string result;
	result.reserve(4);

	for (int i = 0; i < 4; ++i) {
		result.push_back(base64[value & 0x3f]);
		value >>= 6;
	}
	return { result };
}
