#include <MazeBuilder/colored_grid.h>
 
#include <optional>
#include <string>

#include <MazeBuilder/distances.h>
#include <MazeBuilder/cell.h>

#if defined(MAZE_DEBUG)
#include <iostream>
#endif

using namespace mazes;
using namespace std;

/// @brief 
/// @param rows 1
/// @param cols 1
/// @param height 1 
colored_grid::colored_grid(unsigned int rows, unsigned int cols, unsigned int height)
    : grid(rows, cols, height), m_distances(make_shared<distances>(rows * cols)) {

}

void colored_grid::start_configuration(const std::vector<int>& indices) noexcept {
    using namespace std;

    grid::start_configuration(cref(indices));

    try {

        auto [ROWS, COLUMNS, _] = this->get_dimensions();

        auto found = search(ROWS * COLUMNS);

        if (!found) {

            throw std::runtime_error("Search returned a null cell.");
        }

        m_distances = std::make_shared<distances>(found->get_index());

        if (!m_distances) {

            throw std::runtime_error("Failed to create distances object.");
        }

        m_distances = m_distances->path_to(0, *this);

        if (!m_distances) {

            throw std::runtime_error("Failed to get path to goal.");
        }
    } catch (const std::exception& e) {
#if defined(MAZE_DEBUG)
        std::cerr << "Exception in distance_grid::start_configuration: " << e.what() << std::endl;
#endif
    }
}

std::optional<std::string> colored_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
    if (m_distances) {

        if (m_distances->contains(c->get_index())) {

            return std::make_optional(std::to_string(m_distances->operator[](c->get_index())));
        }
    }

    return grid::contents_of(c);
}

optional<uint32_t> colored_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
    if (!c) {

        return nullopt;
    }

	const auto& d = this->m_distances->path_to(c->get_index(), *this);

	if (!d) {

        return grid::background_color_for(cref(c));
	}

	auto max = d->max();

    int distance1 = d->operator[](max.first);

	float intensity = static_cast<float>(10 - distance1) / 10;
	int dark = static_cast<int>(255 * intensity);
	int bright = 128 + static_cast<int>(127 * intensity);
	return (dark << 16) | (bright << 8) | dark;
} 
