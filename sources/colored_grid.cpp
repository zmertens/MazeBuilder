#include "colored_grid.h"

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
    : m_distance_grid{ make_unique<distance_grid>(width, length, height) } {

}

unsigned int colored_grid::get_rows() const noexcept {
    return this->m_distance_grid->get_rows();
}

unsigned int colored_grid::get_columns() const noexcept {
	return this->m_distance_grid->get_columns();
}

/**
 *
 * @param cell_size 25
 */
std::vector<std::uint8_t> colored_grid::to_png(const unsigned int cell_size) const noexcept {
    int png_w = cell_size * this->m_distance_grid->get_columns();
    int png_h = cell_size * this->m_distance_grid->get_rows();
    // Init png data with white background
    vector<uint8_t> png_data((png_w + 1) * (png_h + 1) * 4, 255);

    // Black
    uint32_t wall_color = 0x000000FF;

    // Helper functions to populate png_data
    auto draw_rect = [&png_data, &png_w](int x1, int y1, int x2, int y2, uint32_t color) {
        for (int y = y1; y <= y2; ++y) {
            for (int x = x1; x <= x2; ++x) {
                int index = (y * (png_w + 1) + x) * 4;
                png_data[index] = (color >> 24) & 0xFF;
                png_data[index + 1] = (color >> 16) & 0xFF;
                png_data[index + 2] = (color >> 8) & 0xFF;
                png_data[index + 3] = color & 0xFF;
            }
        }
        };

    auto draw_line = [&png_data, &png_w](int x1, int y1, int x2, int y2, uint32_t color) {
        if (x1 == x2) {
            for (int y = y1; y <= y2; ++y) {
                int index = (y * (png_w + 1) + x1) * 4;
                png_data[index] = (color >> 24) & 0xFF;
                png_data[index + 1] = (color >> 16) & 0xFF;
                png_data[index + 2] = (color >> 8) & 0xFF;
                png_data[index + 3] = color & 0xFF;
            }
        } else if (y1 == y2) {
            for (int x = x1; x <= x2; ++x) {
                int index = (y1 * (png_w + 1) + x) * 4;
                png_data[index] = (color >> 24) & 0xFF;
                png_data[index + 1] = (color >> 16) & 0xFF;
                png_data[index + 2] = (color >> 8) & 0xFF;
                png_data[index + 3] = color & 0xFF;
            }
        }
    };

	auto&& current = this->m_distance_grid->get_root();

    while (current) {
        int x1 = current->get_column() * cell_size;
        int y1 = current->get_row() * cell_size;
        int x2 = (current->get_column() + 1) * cell_size;
        int y2 = (current->get_row() + 1) * cell_size;

        uint32_t color = this->background_color_for(current);

        draw_rect(x1, y1, x2, y2, color);

        if (!current->get_north()) {
            draw_line(x1, y1, x2, y1, wall_color);
        }
        if (!current->get_west()) {
            draw_line(x1, y1, x1, y2, wall_color);
        }
        if (!current->is_linked(current->get_east())) {
            draw_line(x2, y1, x2, y2, wall_color);
        }
        if (!current->is_linked(current->get_south())) {
            draw_line(x1, y2, x2, y2, wall_color);
        }

        current = this->m_distance_grid->search(this->m_distance_grid->get_root(), current->get_index() + 1);
    }

    return png_data;
}

void colored_grid::append(std::unique_ptr<grid> const& other_grid) noexcept {
	this->m_distance_grid->append(other_grid);
}
void colored_grid::insert(std::shared_ptr<cell> const& parent, unsigned int index) noexcept {
	this->m_distance_grid->insert(parent, index);
}
std::shared_ptr<cell> colored_grid::search(std::shared_ptr<cell> const& start, unsigned int index) const noexcept {
	return this->m_distance_grid->search(start, index);
}
void colored_grid::del(std::shared_ptr<cell> parent, unsigned int index) noexcept {
	this->m_distance_grid->del(parent, index);
}

std::shared_ptr<cell> colored_grid::get_root() const noexcept {
	return this->m_distance_grid->get_root();
}

std::string colored_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept {
	return " ";
}

std::uint32_t colored_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept {
    auto distance = this->m_distance_grid->get_distances()->operator[](c);
	auto max = this->m_distance_grid->get_distances()->max().second;
	float intensity = static_cast<float>(max - distance) / max;
	int dark = static_cast<int>(255 * intensity);
	int bright = 128 + static_cast<int>(127 * intensity);
	return (dark << 16) | (bright << 8) | dark;
}