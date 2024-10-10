#include "colored_grid.h"

#include <iostream>
#include <optional>

#include "distance_grid.h"
#include "distances.h"
#include "cell.h"

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

/**
 *
 * @param cell_size 3
 */
std::vector<std::uint8_t> colored_grid::to_pixels(const unsigned int cell_size) const noexcept {
    unsigned int img_width = cell_size * m_distance_grid->get_columns();
    unsigned int img_height = cell_size * m_distance_grid->get_rows();

    uint32_t wall = 0x000000FF;

    // Create an image, RGBA channels, with a white background
    std::vector<uint8_t> img_data(img_width * img_height * 4, 255);

    // Helper functions to draw on the image
    auto draw_rect = [&img_data, img_width](int x1, int y1, int x2, int y2, uint32_t color) {
        for (int y = y1; y < y2; ++y) {
            for (int x = x1; x < x2; ++x) {
                int index = (y * img_width + x) * 4;
                img_data[index] = (color >> 24) & 0xFF;
                img_data[index + 1] = (color >> 16) & 0xFF;
                img_data[index + 2] = (color >> 8) & 0xFF;
                img_data[index + 3] = color & 0xFF;
            }
        }
        };

    auto draw_line = [&img_data, img_width](int x1, int y1, int x2, int y2, uint32_t color) {
        if (x1 == x2) {
            for (int y = y1; y < y2; ++y) {
                int index = (y * img_width + x1) * 4;
                img_data[index] = (color >> 24) & 0xFF;
                img_data[index + 1] = (color >> 16) & 0xFF;
                img_data[index + 2] = (color >> 8) & 0xFF;
                img_data[index + 3] = color & 0xFF;
            }
        } else if (y1 == y2) {
            for (int x = x1; x < x2; ++x) {
                int index = (y1 * img_width + x) * 4;
                img_data[index] = (color >> 24) & 0xFF;
                img_data[index + 1] = (color >> 16) & 0xFF;
                img_data[index + 2] = (color >> 8) & 0xFF;
                img_data[index + 3] = color & 0xFF;
            }
        }
        };

    vector<shared_ptr<cell>> cells;
    cells.reserve(this->get_rows() * this->get_columns());
    this->make_vec(ref(cells));

    // Draw backgrounds and walls
    for (const auto& mode : { "backgrounds", "walls" }) {
        for (const auto& current : cells) {
            int x1 = current->get_column() * cell_size;
            int y1 = current->get_row() * cell_size;
            int x2 = (current->get_column() + 1) * cell_size;
            int y2 = (current->get_row() + 1) * cell_size;

            if (mode == "backgrounds"s) {
                uint32_t color = background_color_for(current).value_or(0xFFFFFFFF);
                draw_rect(x1, y1, x2, y2, color);
            } else {
                if (!current->get_north()) draw_line(x1, y1, x2, y1, wall);
                if (!current->get_west()) draw_line(x1, y1, x1, y2, wall);
                if (auto east = current->get_east(); east && !current->is_linked(cref(east))) draw_line(x2, y1, x2, y2, wall);
                if (auto south = current->get_south(); south && !current->is_linked(cref(south))) draw_line(x1, y2, x2, y2, wall);
            }
        }
    }

    return img_data;
}

void colored_grid::make_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept {
	this->m_distance_grid->make_vec(ref(cells));
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