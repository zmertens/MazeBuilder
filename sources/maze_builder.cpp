#include "maze_builder.h"

#include <sstream>
#include <iostream>
#include <algorithm>
#include <functional>

#include "distance_grid.h"
#include "grid.h"
#include "cell.h"
#include "colored_grid.h"
#include "maze_factory.h"
#include "maze_types_enum.h"

using namespace mazes;
using namespace std;

/**
 * @brief Represent a maze with a thread-safe interface in a 3D grid
 */
maze_builder::maze_builder(unsigned int width, unsigned int length, unsigned int height)
    : m_width(width), m_length(length), m_height(height)
    , m_vertices(), m_faces(), m_p_q(), m_block_type(1) {

}

/**
 * @brief Clear maze objects and parameters
 */
void maze_builder::clear() noexcept {
    m_vertices.clear();
    m_faces.clear();
    m_p_q.clear();
    m_width = 0;
    m_length = 0;
    m_height = 0;
    m_tracker.stop();
}

std::vector<std::tuple<int, int, int, int>> maze_builder::get_render_vertices() const noexcept {
    vector<tuple<int, int, int, int>> render_vertices (this->m_vertices.size() / 8);
    for (size_t i = 0; i < this->m_vertices.size(); i += 8) {
		render_vertices.push_back(this->m_vertices[i]);
	}

    return render_vertices;
}

std::vector<std::tuple<int, int, int, int>> maze_builder::get_writable_vertices() const noexcept {
    return this->m_vertices;
}

std::vector<std::vector<std::uint32_t>> maze_builder::get_faces() const noexcept {
    return this->m_faces;
}

optional<tuple<int, int, int, int>> maze_builder::find_block(int p, int q) const noexcept {
    auto itr = m_p_q.find({ p, q });
    return (itr != m_p_q.cend()) ? make_optional(itr->second) : nullopt;
}

// Return a future for when maze has been written
std::string maze_builder::to_wavefront_obj_str() const noexcept {
    stringstream ss;
    ss << "# https://www.github.com/zmertens/MazeBuilder\n";

    // keep track of writing progress
    int total_verts = static_cast<int>(m_vertices.size());
    int total_faces = static_cast<int>(m_faces.size());

    int t = total_verts + total_faces;
    int c = 0;
    // Write vertices
    for (const auto& vertex : m_vertices) {
        float x = static_cast<float>(get<0>(vertex));
        float y = static_cast<float>(get<1>(vertex));
        float z = static_cast<float>(get<2>(vertex));
        ss << "v " << x << " " << y << " " << z << "\n";
        c++;
    }

    // Write m_faces
    for (const auto& face : m_faces) {
        ss << "f";
        for (auto index : face) {
            ss << " " << index;
        }
        ss << "\n";
        c++;
    }

    return ss.str();
} // to_wavefront_obj_str

/**
 * @brief Generate a PNG image of the maze
 * @param my_maze_type
 * @param get_int
 * @param rng
 * @param cell_size 3
 */
std::vector<std::uint8_t> maze_builder::to_pixels(mazes::maze_types my_maze_type,
    const std::function<int(int, int)>& get_int,
    const std::mt19937& rng, const unsigned int cell_size) const noexcept {

    std::unique_ptr<grid_interface> g = make_unique<colored_grid>(m_width, m_length, m_height);
    bool success = mazes::maze_factory::gen_maze(my_maze_type, ref(g), cref(get_int), cref(rng));

    if (!success) {
        return {};
    }

    auto get_pixels = [&cell_size, &g]() {
        unsigned int img_width = cell_size * g->get_columns();
        unsigned int img_height = cell_size * g->get_rows();

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
        cells.reserve(g->get_rows() * g->get_columns());
        g->make_sorted_vec(ref(cells));

        // Draw backgrounds and walls
        for (const auto& mode : { "backgrounds", "walls" }) {
            for (const auto& current : cells) {
                int x1 = current->get_column() * cell_size;
                int y1 = current->get_row() * cell_size;
                int x2 = (current->get_column() + 1) * cell_size;
                int y2 = (current->get_row() + 1) * cell_size;

                if (mode == "backgrounds"s) {
                    uint32_t color = g->background_color_for(current).value_or(0xFFFFFFFF);
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

    // For colored grids, calculate the distances first
    if (auto ptr = dynamic_cast<colored_grid*>(g.get())) {
        ptr->calc_distances();
#if defined(MAZE_DEBUG)
        cout << "INFO: Calculating distances for colored grid" << endl;
#endif
    }

    return get_pixels();
}

void  maze_builder::set_height(unsigned int height) noexcept {
    this->m_height = height;
}

unsigned int  maze_builder::get_height() const noexcept {
    return this->m_height;
}

void  maze_builder::set_length(unsigned int length) noexcept {
    this->m_length = length;
}

unsigned int  maze_builder::get_length() const noexcept {
    return this->m_length;
}

void  maze_builder::set_width(unsigned int width) noexcept {
    this->m_width = width;
}

unsigned int  maze_builder::get_width() const noexcept {
    return this->m_width;
}

void maze_builder::start_progress() noexcept {
    this->m_tracker.start();
}

void maze_builder::stop_progress() noexcept {
    this->m_tracker.stop();
}

double maze_builder::get_progress_in_seconds() const noexcept {
    return this->m_tracker.get_duration_in_seconds();
}

double maze_builder::get_progress_in_ms() const noexcept {
    return this->m_tracker.get_duration_in_ms();
}

std::size_t maze_builder::get_vertices_size() const noexcept {
    return this->m_vertices.size();
}

/**
 * @brief
 * @param calc_distances false
 */
std::string maze_builder::to_str(maze_types my_maze_type,
    const std::function<int(int, int)>& get_int, const std::mt19937& rng,
    bool calc_distances) const noexcept {
    
    std::unique_ptr<grid_interface> g = nullptr;
    if (calc_distances) {
        g = make_unique<mazes::distance_grid>(m_width, m_length, m_height);
    } else {
        g = make_unique<mazes::grid>(m_width, m_length, m_height);
    }

    bool success = mazes::maze_factory::gen_maze(my_maze_type, ref(g), cref(get_int), cref(rng));

    if (!success) {
        return "";
    }

    stringstream ss;
    if (calc_distances) {
        if (auto distance_ptr = dynamic_cast<distance_grid*>(g.get())) {
#if defined(MAZE_DEBUG)
            cout << "INFO: Calculating distances" << endl;
#endif
            distance_ptr->calc_distances();
            ss << *distance_ptr;
            return ss.str();
        } 
    } else if (auto grid_ptr = dynamic_cast<grid*>(g.get())) {
        ss << *grid_ptr;
        return ss.str();
    } 
    return "";
} // to_str

/**
 * @brief Parses the grid, and builds a 3D grid using (x, y, z, w) (w == block type)
 * @param block_type = 1
 * @return
*/
void maze_builder::compute_geometry(maze_types my_maze_type,
    const std::function<int(int, int)>& get_int, const std::mt19937& rng,
    int block_type) noexcept {
    
    istringstream iss{ this->to_str(my_maze_type, cref(get_int), cref(rng)) };
    string line;
    unsigned int row_x = 0;
    while (getline(iss, line, '\n')) {
        unsigned int col_z = 0;
        for (auto itr = line.cbegin(); itr != line.cend() && col_z < line.size(); itr++) {
            // Check for barriers and walls then iterate up to the height of the maze
            if (*itr == MAZE_CORNER || *itr == MAZE_BARRIER1 || *itr == MAZE_BARRIER2) {
                static constexpr unsigned int block_size = 1;
                for (auto h{ 0 }; h < m_height; h++) {
                    // Update the data source that stores the maze geometry
                    // There are 2 data sources, one for rendering and one for writing
                    block_type = (block_type == -1) ? get_int(1, 10) : block_type;
                    this->add_block(row_x, h, col_z, block_type, block_size);
                    m_p_q[{row_x, col_z}] = make_tuple(row_x, h, col_z, block_type);
                }
            }
            col_z++;
        }
        row_x++;
    } // getline
} // compute_geometry

/**
 * @brief Update the maze with a new block, updating geometric write and render vertices
 *  Write vertices are used for Wavefront OBJ file writing
 *  Render vertices are used for rendering in Craft
 */
void maze_builder::add_block(int x, int y, int z, int w, int block_size) noexcept {
    // Calculate the base index for the new vertices
    // OBJ format is 1-based indexing
    std::uint32_t baseIndex = static_cast<std::uint32_t>(this->m_vertices.size() + 1);
    // Define the 8 vertices of the cube
    this->m_vertices.emplace_back(x, y, z, w);
    this->m_vertices.emplace_back(x + block_size, y, z, w);
    this->m_vertices.emplace_back(x + block_size, y + block_size, z, w);
    this->m_vertices.emplace_back(x, y + block_size, z, w);
    this->m_vertices.emplace_back(x, y, z + block_size, w);
    this->m_vertices.emplace_back(x + block_size, y, z + block_size, w);
    this->m_vertices.emplace_back(x + block_size, y + block_size, z + block_size, w);
    this->m_vertices.emplace_back(x, y + block_size, z + block_size, w);

    // Define m_faces using the vertices above (12 triangles for 6 m_faces)
    // Front face
    this->m_faces.push_back({ {baseIndex, baseIndex + 1, baseIndex + 2} });
    this->m_faces.push_back({ {baseIndex, baseIndex + 2, baseIndex + 3} });
    // Back face
    this->m_faces.push_back({ {baseIndex + 4, baseIndex + 6, baseIndex + 5} });
    this->m_faces.push_back({ {baseIndex + 4, baseIndex + 7, baseIndex + 6} });
    // Left face
    this->m_faces.push_back({ {baseIndex, baseIndex + 3, baseIndex + 7} });
    this->m_faces.push_back({ {baseIndex, baseIndex + 7, baseIndex + 4} });
    // Right face
    this->m_faces.push_back({ {baseIndex + 1, baseIndex + 5, baseIndex + 6} });
    this->m_faces.push_back({ {baseIndex + 1, baseIndex + 6, baseIndex + 2} });
    // Top face
    this->m_faces.push_back({ {baseIndex + 3, baseIndex + 2, baseIndex + 6} });
    this->m_faces.push_back({ {baseIndex + 3, baseIndex + 6, baseIndex + 7} });
    // Bottom face
    this->m_faces.push_back({ {baseIndex, baseIndex + 4, baseIndex + 5} });
    this->m_faces.push_back({ {baseIndex, baseIndex + 5, baseIndex + 1} });
} // add_block
