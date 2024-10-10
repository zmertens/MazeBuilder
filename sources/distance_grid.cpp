#include "distance_grid.h"

#include "distances.h"
#include "grid.h"
#include "cell.h"

#include <queue>
#include <cpp-base64/base64.h>

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

void distance_grid::make_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept {
	return this->grid::make_vec(ref(cells));
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
		if (d >= 0) {
			return to_base64(d);
		}
	}
	return grid::contents_of(c);
}

std::optional<std::uint32_t> distance_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
	return grid::background_color_for(cref(c));
}

optional<std::string> distance_grid::to_base64(int value) const {
	//string value_str = to_string(value);
	//return base64_encode(reinterpret_cast<const unsigned char*>(value_str.c_str()), value_str.size());
	static constexpr auto base36_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::string result;
	do {
		result.push_back(base36_chars[value % 36]);
		value /= 36;
	} while (value > 0);
	std::reverse(result.begin(), result.end());
	return result;
}

void distance_grid::calc_distances() noexcept {
	auto&& root = this->get_root();
	queue<shared_ptr<cell>> frontier;
	frontier.push(root);
	m_distances->set(root, 0);
	// apply shortest path algorithm
	while (!frontier.empty()) {
		auto current = frontier.front();
		frontier.pop();
		auto current_distance = m_distances->operator[](current);
		for (const auto& neighbor : current->get_neighbors()) {
			if (!m_distances->contains(neighbor)) {
				m_distances->set(neighbor, m_distances->operator[](current) + 1);
				frontier.push(neighbor);
			}
		}
	}
}