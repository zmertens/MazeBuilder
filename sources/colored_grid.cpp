#include <MazeBuilder/colored_grid.h>

#include <iostream>
#include <optional>

#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/cell.h>

using namespace mazes;
using namespace std;

/**
 * @brief Construct a new distance grid object
 * @param width
 * @param length
 * @param height 0
 */
colored_grid::colored_grid(unsigned int width, unsigned int length, unsigned int height)
    : m_distance_grid(make_unique<distance_grid>(width, length, height)) {

}

unsigned int colored_grid::get_rows() const noexcept {
    return m_distance_grid->get_rows();
}

unsigned int colored_grid::get_columns() const noexcept {
	return m_distance_grid->get_columns();
}

void colored_grid::make_sorted_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept {
	this->m_distance_grid->make_sorted_vec(ref(cells));
}

void colored_grid::append(std::unique_ptr<grid_interface> const& other_grid) noexcept {
	m_distance_grid->append(other_grid);
}
void colored_grid::insert(std::shared_ptr<cell> const& parent, int index) noexcept {
	m_distance_grid->insert(parent, index);
}

bool colored_grid::update(std::shared_ptr<cell>& parent, int old_index, int new_index) noexcept {
	return m_distance_grid->update(ref(parent), old_index, new_index);
}

std::shared_ptr<cell> colored_grid::search(std::shared_ptr<cell> const& start, int index) const noexcept {
	return m_distance_grid->search(start, index);
}
void colored_grid::del(std::shared_ptr<cell> parent, int index) noexcept {
	m_distance_grid->del(parent, index);
}

std::shared_ptr<cell> colored_grid::get_root() const noexcept {
	return m_distance_grid->get_root();
}

std::optional<std::string> colored_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
	return m_distance_grid->contents_of(cref(c));
}

optional<uint32_t> colored_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {	
	const auto& dists = m_distance_grid->get_distances()->path_to(c);
	if (!dists) {
		return nullopt;
	}

	auto max = dists->max();

	int distance1 = dists->operator[](c);
	float intensity = static_cast<float>(10 - distance1) / 10;
	int dark = static_cast<int>(255 * intensity);
	int bright = 128 + static_cast<int>(127 * intensity);
	return (dark << 16) | (bright << 8) | dark;
	//return m_distance_grid->background_color_for(cref(c));
}

void colored_grid::calc_distances() noexcept {
	m_distance_grid->calc_distances();
}