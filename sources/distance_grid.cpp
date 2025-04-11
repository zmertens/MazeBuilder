#include <MazeBuilder/distance_grid.h>

#include <MazeBuilder/distances.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/cell.h>

#include <queue>
#include <random>
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
distance_grid::distance_grid(unsigned int rows, unsigned int cols, unsigned int height)
	: grid::grid(rows, cols, height) {

}

/// @brief Constructs a distance_grid object with specified dimensions and initializes the distance calculations.
/// @return future to init task
void distance_grid::start_configuration(const std::vector<int>& indices) noexcept {
    using namespace std;

    grid::start_configuration(cref(indices));

    try {

        auto [ROWS, COLUMNS, _] = this->get_dimensions();

        auto found = search(ROWS * COLUMNS - 1);

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

std::optional<std::string> distance_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
	if (m_distances) {

        if (m_distances->contains(c->get_index())) {

            const auto d = m_distances->operator[](c->get_index());
            if (d >= 0) {

                return to_base36(d);
            }
        }
	}

	return grid::contents_of(c);
}

std::optional<std::uint32_t> distance_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
	return grid::background_color_for(cref(c));
}

std::optional<std::string> distance_grid::to_base36(int value) const {
	static constexpr auto base36_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::string result;
	do {
		result.push_back(base36_chars[value % 36]);
		value /= 36;
	} while (value > 0);
	std::reverse(result.begin(), result.end());
	return result;
}

std::shared_ptr<distances> distance_grid::get_distances() const noexcept {
	return this->m_distances;
}
