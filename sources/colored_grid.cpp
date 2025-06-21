#include <MazeBuilder/colored_grid.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_operations.h>

#include <functional>
#include <string>

#if defined(MAZE_DEBUG)
#include <iostream>
#endif

using namespace mazes;
using namespace std;

/// @brief 
/// @param rows 1
/// @param cols 1
/// @param levels 1
colored_grid::colored_grid(unsigned int rows, unsigned int cols, unsigned int levels)
    : m_grid{ std::make_unique<grid>(rows, cols, levels) }
    , m_distances(make_shared<distances>(rows * cols)) {

}

//void colored_grid::configure(const std::vector<int>& indices) noexcept {
//
//    using namespace std;

//    grid::configure(cref(indices));
//
//    try {
//
//        auto [ROWS, COLUMNS, _] = this->get_dimensions();
//
//        auto found = search(ROWS * COLUMNS);
//
//        if (!found) {
//
//            throw std::runtime_error("Search returned a null cell.");
//        }
//
//        m_distances = std::make_shared<distances>(found->get_index());
//
//        if (!m_distances) {
//
//            throw std::runtime_error("Failed to create distances object.");
//        }
//
//        m_distances = m_distances->path_to(0, *this);
//
//        if (!m_distances) {
//
//            throw std::runtime_error("Failed to get path to goal.");
//        }
//    } catch (const std::exception& e) {
//#if defined(MAZE_DEBUG)
//        std::cerr << "Exception in distance_grid::start_configuration: " << e.what() << std::endl;
//#endif
//    }
//}

std::string colored_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
    if (m_distances) {

        if (m_distances->contains(c->get_index())) {

            return std::to_string(m_distances->operator[](c->get_index()));
        }
    }

    // Fall back to default representation if no distance info available
    return m_grid->contents_of(c);
}

std::uint32_t colored_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {

    using namespace std;

    if (!c) {

        return m_grid->background_color_for(cref(c));
    }

	const auto& d = this->m_distances->path_to(cref(m_grid), c->get_index());

	if (!d) {

        return m_grid->background_color_for(cref(c));
	}

	auto max = d->max();

    int distance1 = d->operator[](max.first);

	float intensity = static_cast<float>(10 - distance1) / 10;
	int dark = static_cast<int>(255 * intensity);
	int bright = 128 + static_cast<int>(127 * intensity);
	return (dark << 16) | (bright << 8) | dark;
} 

// Delegate to embedded grid
grid_operations& colored_grid::operations() noexcept {

    return m_grid->operations();
}

const grid_operations& colored_grid::operations() const noexcept {

    return m_grid->operations();
}
