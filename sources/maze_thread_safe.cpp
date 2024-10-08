#include "maze_thread_safe.h"

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
maze_thread_safe::maze_thread_safe(unsigned int width, unsigned int length, unsigned int height)
    : m_width(width), m_length(length), m_height(height)
    , m_vertices(), m_faces(), m_p_q(), m_block_type(1) {

}

/**
 * @brief Clear maze objects and parameters
 */
void maze_thread_safe::clear() noexcept {
    std::lock_guard<std::mutex> lock(m_maze_mutx);
    std::lock_guard<std::shared_mutex> lock_verts(m_verts_mtx);
    m_vertices.clear();
    m_faces.clear();
    m_p_q.clear();
    m_width = 0;
    m_length = 0;
    m_height = 0;
    m_tracker.stop();
}

std::vector<std::tuple<int, int, int, int>> maze_thread_safe::get_render_vertices() const noexcept {
    std::shared_lock<std::shared_mutex> lock(m_verts_mtx);

    vector<tuple<int, int, int, int>> render_vertices (this->m_vertices.size() / 8);
    for (size_t i = 0; i < this->m_vertices.size(); i += 8) {
		render_vertices.push_back(this->m_vertices[i]);
	}

    return render_vertices;
}

std::vector<std::tuple<int, int, int, int>> maze_thread_safe::get_writable_vertices() const noexcept {
    std::shared_lock<std::shared_mutex> lock(m_verts_mtx);
    return this->m_vertices;
}

std::vector<std::vector<std::uint32_t>> maze_thread_safe::get_faces() const noexcept {
    std::shared_lock<std::shared_mutex> lock(m_verts_mtx);
    return this->m_faces;
}

optional<tuple<int, int, int, int>> maze_thread_safe::find_block(int p, int q) noexcept {
    std::lock_guard<std::mutex> lock(m_maze_mutx);

    auto itr = m_p_q.find({ p, q });
    return (itr != m_p_q.cend()) ? make_optional(itr->second) : nullopt;
}

// Return a future for when maze has been written
std::string maze_thread_safe::to_wavefront_obj_str() const noexcept {
    using namespace std;
    std::lock_guard<std::shared_mutex> lock(m_verts_mtx);
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
std::vector<std::uint8_t> maze_thread_safe::to_pixels(mazes::maze_types my_maze_type,
    const std::function<int(int, int)>& get_int,
    const std::mt19937& rng, const unsigned int cell_size) const noexcept {

    std::unique_ptr<grid_interface> g = make_unique<colored_grid>(m_width, m_length, m_height);
    bool success = mazes::maze_factory::gen_maze(my_maze_type, ref(g), cref(get_int), cref(rng));

    if (!success) {
        // Handle error
    }

    if (auto ptr = dynamic_cast<colored_grid*>(g.get())) {
        ptr->calc_distances();
#if defined(MAZE_DEBUG)
        cout << "Calc distances for colored grid" << endl;
#endif
        return ptr->to_pixels(cell_size);
    }

    return g->to_pixels(cell_size);
}

void  maze_thread_safe::set_height(unsigned int height) noexcept {
    this->m_height = height;
}

unsigned int  maze_thread_safe::get_height() const noexcept {
    return this->m_height;
}

void  maze_thread_safe::set_length(unsigned int length) noexcept {
    this->m_length = length;
}

unsigned int  maze_thread_safe::get_length() const noexcept {
    return this->m_length;
}

void  maze_thread_safe::set_width(unsigned int width) noexcept {
    this->m_width = width;
}

unsigned int  maze_thread_safe::get_width() const noexcept {
    return this->m_width;
}

void maze_thread_safe::start_progress() noexcept {
    this->m_tracker.start();
}

void maze_thread_safe::stop_progress() noexcept {
    this->m_tracker.stop();
}

double maze_thread_safe::get_progress_in_seconds() const noexcept {
    return this->m_tracker.get_duration_in_seconds();
}

double maze_thread_safe::get_progress_in_ms() const noexcept {
    return this->m_tracker.get_duration_in_ms();
}

std::size_t maze_thread_safe::get_vertices_size() const noexcept {
    std::shared_lock<std::shared_mutex> lock(m_verts_mtx);
    return this->m_vertices.size();
}

/**
 * @brief
 * @param calc_distances false
 */
std::string maze_thread_safe::to_str(maze_types my_maze_type,
    const std::function<int(int, int)>& get_int, const std::mt19937& rng,
    bool calc_distances) const noexcept {
    
    std::unique_ptr<grid_interface> g = nullptr;
    if (calc_distances) {
        g = make_unique<mazes::distance_grid>(m_width, m_length, m_height);
    } else {
        g = make_unique<mazes::grid>(m_width, m_length, m_height);
    }
    cout << "BEfore maze gen" << endl;
    bool success = mazes::maze_factory::gen_maze(my_maze_type, ref(g), cref(get_int), cref(rng));
    cout << "After maze gen" << endl;
    if (!success) {
        return "";
    }

    stringstream ss;
    if (calc_distances) {
        if (auto distance_ptr = dynamic_cast<distance_grid*>(g.get())) {
#if defined(MAZE_DEBUG)
            cout << "Calculating distances" << endl;
#endif
            distance_ptr->calc_distances();
            ss << distance_ptr;
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
void maze_thread_safe::compute_geometry(maze_types my_maze_type,
    const std::function<int(int, int)>& get_int, const std::mt19937& rng,
    int block_type) noexcept {
    
    lock_guard<mutex> lock(m_maze_mutx);
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
void maze_thread_safe::add_block(int x, int y, int z, int w, int block_size) noexcept {
    std::lock_guard<std::shared_mutex> lock(m_verts_mtx);
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
