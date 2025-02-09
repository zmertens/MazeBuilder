#include <MazeBuilder/maze.h>

#include <sstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <random>

#include <MazeBuilder/maze_builder.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/factory.h>

#include <cpp-base64/base64.h>
#include <nlohmann/json.hpp>

using namespace mazes;
using namespace std;

maze::maze() 
: my_grid(make_unique<grid>(0, 0, 0))
, rows(0)
, columns(0)
, height(0)
, maze_type(mazes::algos::INVALID_ALGO)
, seed(0)
, distances(false)
, block_type(0)
, offset_x(0)
, offset_z(0) {
    
}

void maze::init() noexcept {
    using namespace std;

    mt19937 mt(seed);
    auto get_int = [&mt](int min, int max) {
        uniform_int_distribution<int> dist(min, max);
        return dist(mt);
    };

    if (this->distances) {
        my_grid = make_unique<colored_grid>(this->rows, this->columns, this->height);
    } else {
        my_grid = make_unique<grid>(this->rows, this->columns, this->height);
    }

    bool success = factory::gen(this->maze_type, ref(my_grid), cref(get_int), cref(mt));
    if (!success) {
    }

}

std::vector<std::tuple<int, int, int, int>> maze::get_render_vertices() const noexcept {
    vector<tuple<int, int, int, int>> render_vertices (this->vertices.size() / 8);
    for (size_t i = 0; i < this->vertices.size(); i += 8) {
		render_vertices.push_back(this->vertices[i]);
	}

    return render_vertices;
}

vector<maze::dimensions> maze::get_writable_vertices() const noexcept {
    return this->vertices;
}

std::vector<std::vector<std::uint32_t>> maze::get_faces() const noexcept {
    return this->faces;
}

optional<tuple<int, int, int, int>> maze::maze::find_block(int p, int q) const noexcept {
    auto itr = m_p_q.find({ p, q });
    return (itr != m_p_q.cend()) ? make_optional(itr->second) : nullopt;
}

void maze::maze::populate_cells(std::vector<std::shared_ptr<mazes::cell>>& cells) const noexcept {
    this->my_grid->populate_vec(ref(cells));
}

// Return a future for when maze has been written
std::string maze::maze::to_wavefront_obj_str() const noexcept {
    stringstream ss;
    ss << "# https://www.github.com/zmertens/MazeBuilder\n";

    // keep track of writing progress
    int total_verts = static_cast<int>(vertices.size());
    int total_faces = static_cast<int>(faces.size());

    int t = total_verts + total_faces;
    int c = 0;
    // Write vertices
    for (const auto& vertex : vertices) {
        float x = static_cast<float>(get<0>(vertex));
        float y = static_cast<float>(get<1>(vertex));
        float z = static_cast<float>(get<2>(vertex));
        ss << "v " << x << " " << y << " " << z << "\n";
        c++;
    }

    // Write faces
    for (const auto& face : faces) {
        ss << "f";
        for (auto index : face) {
            ss << " " << index;
        }
        ss << "\n";
        c++;
    }

    return ss.str();
} // to_wavefront_obj_str

std::string maze::maze::to_wavefront_obj_str64() const noexcept {
    return base64_encode(this->to_wavefront_obj_str());
}

/**
 * @brief Generate a PNG image of the maze
 * @param cell_size 3
 */
std::vector<std::uint8_t> maze::maze::to_pixels(unsigned int cell_size) const noexcept {

    auto get_pixels = [this, &cell_size]() {
         int img_width = cell_size * this->columns;
         int img_height = cell_size * this->rows;

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

        vector<shared_ptr<mazes::cell>> cells;
        cells.reserve(this->rows * this->columns);
        this->my_grid->populate_vec(ref(cells));

        // Draw backgrounds and walls
        for (const auto& mode : { "backgrounds", "walls" }) {
            for (const auto& current : cells) {
                int x1 = current->get_column() * cell_size;
                int y1 = current->get_row() * cell_size;
                int x2 = (current->get_column() + 1) * cell_size;
                int y2 = (current->get_row() + 1) * cell_size;

                if (mode == "backgrounds"s) {
                    uint32_t color = my_grid->background_color_for(current).value_or(0xFFFFFFFF);
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
        }; // lambda

    // @TODO Placeholder in case a specific colored_grid function is needed
    //if (auto ptr = dynamic_cast<colored_grid*>(g.get())) {
    //}

    return get_pixels();
}

/**
 * @brief Return a JSON string with base64 representation of the maze
 * @param pretty_spaces = 4
 */
std::string maze::maze::to_json_str(unsigned int pretty_spaces) const noexcept {

    nlohmann::json my_json;
    my_json["str64"] = this->to_str64();
    my_json["obj64"] = this->to_wavefront_obj_str64();
    my_json["num_cols"] = this->columns;
    my_json["num_rows"] = this->rows;
    my_json["height"] = this->height;
    my_json["algo"] = mazes::to_string(this->maze_type);
    my_json["seed"] = this->seed;
    my_json["v"] = mazes::VERSION;

    return my_json.dump(pretty_spaces);
}

/**
 * @brief Return a JSON string with base64 representation of the mazes
 * @param mazes container of mazes
 * @param pretty_spaces = 4
 *
 */
std::string maze::to_json_array_str(const vector<maze_ptr>& mazes, unsigned int pretty_spaces) noexcept {
    nlohmann::json json_array = nlohmann::json::array();

    for (const auto& m : mazes) {
        json_array.push_back(nlohmann::json::parse(m->to_json_str()));
    }

    return json_array.dump(pretty_spaces);
}

/**
 * @brief
 * @param show_distances false
 */
std::string maze::maze::to_str() const noexcept {
    using namespace std;
    stringstream ss;
    ss << *my_grid;
    return ss.str();
} // to_str

std::string maze::maze::to_str64() const noexcept {
    stringstream ss64;
    ss64 << *my_grid;
    auto s64{ ss64.str() };
    return base64_encode(cref(s64));
}

void maze::intopq(int x, int y, int z, int w) noexcept {
    m_p_q[{x, z}] = make_tuple(x, y, z, w);
}

std::optional<std::reference_wrapper<const std::unique_ptr<grid_interface>>> maze::get_grid() const noexcept {
    if (this->my_grid) {
        return cref(this->my_grid);
    }
    return std::nullopt;
}