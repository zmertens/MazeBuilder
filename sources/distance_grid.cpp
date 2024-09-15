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
	: m_grid(make_unique<grid>(width, length, height)) {

}

void distance_grid::set_distances(std::shared_ptr<distances> d) noexcept {
	this->m_distances = d;
}

std::shared_ptr<distances> distance_grid::get_distances() const noexcept {
	return this->m_distances;
}

unsigned int distance_grid::get_rows() const noexcept {
	return this->m_grid->get_rows();
}

unsigned int distance_grid::get_columns() const noexcept {
	return this->m_grid->get_columns();
}

/**
 *
 * @param cell_size 25
 */
std::vector<std::uint8_t> distance_grid::to_png(const unsigned int cell_size) const noexcept {
	return this->m_grid->to_png(cell_size);
}

void distance_grid::append(std::unique_ptr<grid_interface> const& other_grid) noexcept {
	this->m_grid->append(other_grid);
}
void distance_grid::insert(std::shared_ptr<cell> const& parent, unsigned int index) noexcept {
	this->m_grid->insert(parent, index);
}
std::shared_ptr<cell> distance_grid::search(std::shared_ptr<cell> const& start, unsigned int index) const noexcept {
	return this->m_grid->search(start, index);
}
void distance_grid::del(std::shared_ptr<cell> parent, unsigned int index) noexcept {
	this->m_grid->del(parent, index);
}

std::shared_ptr<cell> distance_grid::get_root() const noexcept {
	return m_grid->get_root();
}

std::string distance_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
	if (m_distances) {
		const auto d = m_distances->operator[](c);
		return d >= 0 ? to_base64(d) : m_grid->contents_of(c);
	}
	return m_grid->contents_of(c);
}


std::uint32_t distance_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
	return 0;
}

std::string distance_grid::to_base64(int value) const {
	static const std::string base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string result;
	result.reserve(4);

	for (int i = 0; i < 4; ++i) {
		result.push_back(base64[value & 0x3f]);
		value >>= 6;
	}
	return result;
}

const unique_ptr<grid>& distance_grid::get_grid() const noexcept {
	return m_grid;
}