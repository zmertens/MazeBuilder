#include "maze_thread_safe.h"

#include <sstream>
#include <iostream>

#include "grid.h"
#include "maze_factory.h"
#include "maze_types_enum.h"

using namespace mazes;
using namespace std;

/**
 * @brief Represent a maze with a thread-safe interface in a 3D grid
 */
maze_thread_safe::maze_thread_safe(mazes::maze_types my_maze_type,
    const std::function<int(int, int)>& get_int, const std::mt19937& rng,
    unsigned int width, unsigned int length, unsigned int height, unsigned int block_type)
    : m_maze(), m_width(width), m_length(length), m_height(height), m_block_type(block_type)
    , m_vertices(), m_faces(), m_p_q() {
    this->m_maze = this->compute_str(my_maze_type, get_int, rng);
    this->compute_geometry(this->m_block_type);
}

void maze_thread_safe::set_maze(const std::string& maze) noexcept {
    std::lock_guard<std::mutex> lock(m_maze_mutx);
    this->m_maze = maze;
}

std::string maze_thread_safe::get_maze() noexcept {
    std::lock_guard<std::mutex> lock(m_maze_mutx);
    return this->m_maze;
}

/**
 * @brief Clear maze objects and parameters
 */
void maze_thread_safe::clear() noexcept {
    std::lock_guard<std::mutex> lock(m_maze_mutx);
    std::lock_guard<std::shared_mutex> lock_verts(m_verts_mtx);
    m_maze.clear();
    m_vertices.clear();
    m_faces.clear();
    m_p_q.clear();
    m_width = 0;
    m_length = 0;
    m_height = 0;
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

const maze_thread_safe::pqmap& maze_thread_safe::get_p_q() const noexcept {
	return this->m_p_q;
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

#if defined(MAZE_DEBUG)
    cout << "INFO: Writing maze progress: " << c << "/" << t << "\n";
#endif

    return ss.str();
} // to_wavefront_obj_str

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

void maze_thread_safe::set_block_type(unsigned int block_type) noexcept {
    this->m_block_type = block_type;
}

unsigned int maze_thread_safe::get_block_type() const noexcept {
    return this->m_block_type;
}

std::string maze_thread_safe::compute_str(maze_types my_maze_type,
    const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept {
    auto g = make_unique<mazes::grid>(m_width, m_length, m_height);
    bool success = mazes::maze_factory::gen_maze(my_maze_type, ref(g), cref(get_int), cref(rng));

    if (!success) {
        return "";
    }

    stringstream ss;
    ss << *g.get();
    return ss.str();
} // to_str

/**
 * @brief Parses the grid, and builds a 3D grid using (x, y, z, w) (w == block type)
 * @param block_type = 1
 * @return
*/
void maze_thread_safe::compute_geometry(unsigned int block_type) noexcept {
    lock_guard<mutex> lock(m_maze_mutx);
    istringstream iss{ m_maze.data() };
    string line;
    unsigned int row_x = 0;
    while (getline(iss, line, '\n')) {
        unsigned int col_z = 0;
        for (auto itr = line.cbegin(); itr != line.cend() && col_z < line.size(); itr++) {
            // Check for barriers and walls then iterate up to the height of the maze
            if (*itr == MAZE_CORNER || *itr == MAZE_BARRIER1 || *itr == MAZE_BARRIER2) {
                static constexpr unsigned int starting_height = 5u;
                static constexpr unsigned int block_size = 1;
                for (auto h{ starting_height }; h < starting_height + m_height; h++) {
                    // Update the data source that stores the maze geometry
                    // There are 2 data sources, one for rendering and one for writing
                    this->add_block(row_x, h, col_z, block_type, block_size);
                }
                m_p_q[{ row_x, col_z}] = true;
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
