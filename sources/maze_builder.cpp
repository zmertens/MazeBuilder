#include <MazeBuilder/maze_builder.h>

#include <sstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <random>

#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/maze_factory.h>
#include <MazeBuilder/maze_types_enum.h>

#include <cpp-base64/base64.h>
#include <nlohmann/json.hpp>

using namespace mazes;
using namespace std;

/**
 * @brief Represent a maze with a thread-safe interface in a 3D grid
 * @param show_distances = false
 * @param block_type = -1
 */
maze_builder::maze_builder(int rows,  int columns,  int height, bool show_distances, int block_type)
    : m_grid{}
    , m_maze_type(maze_types::BINARY_TREE), m_seed(0)
    , m_vertices(), m_faces(), m_p_q(), m_show_distances(show_distances), m_block_type(block_type) {

    if (m_show_distances) {
        m_grid = make_unique<colored_grid>(rows, columns, height);
    } else {
        m_grid = make_unique<grid>(rows, columns, height);
    }

    mt19937 rng(random_device{}());
    auto get_int = [&rng](int low, int high)->int {
        uniform_int_distribution<int> dist(low, high); return dist(rng); };

    bool success = mazes::maze_factory::gen(m_maze_type, ref(m_grid), cref(get_int), cref(rng));
    if (!success) {
    }

    // Compute 3D geometries (includes height)
    this->compute_geometry(m_maze_type, cref(get_int), cref(rng));
}

/**
 * @brief Constructor with all the bells and whistles
 * @param block_type = -1
 * @param show_distances = false
 */
maze_builder::maze_builder(int rows, int cols, int height,
    mazes::maze_types my_maze_type,
    const std::function<int(int, int)>& get_int,
    const std::mt19937& rng,
    bool show_distances,
    int block_type) : m_grid{}
    , m_maze_type(my_maze_type), m_seed(static_cast<int>(rng.default_seed))
    , m_vertices(), m_faces(), m_p_q(), m_show_distances(show_distances), m_block_type(block_type) {

    if (m_show_distances) {
        m_grid = make_unique<colored_grid>(rows, cols, height);
    } else {
        m_grid = make_unique<grid>(rows, cols, height);
    }

    bool success = mazes::maze_factory::gen(my_maze_type, ref(m_grid), cref(get_int), cref(rng));

    if (!success) {
    }

    this->compute_geometry(my_maze_type, cref(get_int), cref(rng));
}

/**
 * @brief Clear maze objects and parameters
 */
void maze_builder::clear() noexcept {
    m_vertices.clear();
    m_faces.clear();
    m_p_q.clear();
    m_tracker.stop();
    m_grid.reset();
    m_show_distances = false;
    m_maze_type = maze_types::BINARY_TREE;
    m_block_type = 1;
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
 * @param cell_size 3
 */
std::vector<std::uint8_t> maze_builder::to_pixels(unsigned int cell_size) const noexcept {

    auto get_pixels = [this, &cell_size]() {
         int img_width = cell_size * this->get_columns();
         int img_height = cell_size * this->get_rows();

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
        cells.reserve(this->get_rows() * this->get_columns());
        m_grid->populate_vec(ref(cells));

        // Draw backgrounds and walls
        for (const auto& mode : { "backgrounds", "walls" }) {
            for (const auto& current : cells) {
                int x1 = current->get_column() * cell_size;
                int y1 = current->get_row() * cell_size;
                int x2 = (current->get_column() + 1) * cell_size;
                int y2 = (current->get_row() + 1) * cell_size;

                if (mode == "backgrounds"s) {
                    uint32_t color = m_grid->background_color_for(current).value_or(0xFFFFFFFF);
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
std::string maze_builder::to_json_str(unsigned int pretty_spaces) const noexcept {

    nlohmann::json my_json;
    my_json["output"] = this->to_str64();
    my_json["num_cols"] = this->get_columns();
    my_json["num_rows"] = this->get_rows();
    my_json["depth"] = this->get_height();
    my_json["algorithm"] = mazes::to_string(m_maze_type);
    my_json["seed"] = m_seed;

    return my_json.dump(4);
}

int maze_builder::get_height() const noexcept {
    return this->m_grid->get_height();
}

int maze_builder::get_columns() const noexcept {
    return this->m_grid->get_columns();
}

int maze_builder::get_rows() const noexcept {
    return this->m_grid->get_rows();
}

int maze_builder::get_block_type() const noexcept {
    return this->m_block_type;
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
 * @param show_distances false
 */
std::string maze_builder::to_str() const noexcept {
    stringstream ss;
    ss << *m_grid;
    return ss.str();
} // to_str

std::string maze_builder::to_str64() const noexcept {

    stringstream ss64;
    ss64 << *m_grid;
    auto s64{ ss64.str() };
    return base64_encode(cref(s64));
}

/**
 * @brief Parses the grid, and builds a 3D grid using (x, y, z, w) (w == block type)
*/
void maze_builder::compute_geometry(maze_types my_maze_type, const std::function<int(int, int)>& get_int, const std::mt19937& rng) noexcept {
    bool use_get_int = (m_block_type == -1) ? true : false;
    istringstream iss{ this->to_str() };
    string line;
    int row_x = 0;
    while (getline(iss, line, '\n')) {
        int col_z = 0;
        for (auto itr = line.cbegin(); itr != line.cend() && col_z < line.size(); itr++) {
            // Check for barriers and walls then iterate up to the height of the maze
            if (*itr == MAZE_CORNER || *itr == MAZE_BARRIER1 || *itr == MAZE_BARRIER2) {
                static constexpr  int block_size = 1;
                for (auto h{ 0 }; h < m_grid->get_height(); h++) {
                    // Update the data source that stores the maze geometry
                    // There are 2 data sources, one for rendering and one for writing
                    int block_type = (use_get_int) ? get_int(0, 12) : m_block_type;
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
