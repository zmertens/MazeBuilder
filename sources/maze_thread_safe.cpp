#include "maze_thread_safe.h"

#include <sstream>
#include <iostream>

using namespace mazes;
using namespace std;

/**
 * @brief Represent a maze with a thread-safe interface in a 3D grid
 */
maze_thread_safe::maze_thread_safe(const std::string& maze, unsigned int height) 
    : m_maze(maze), m_height(height)
    , m_wavefront_obj_vertices(), m_render_vertices(), m_faces() {
    this->compute_geometry();
}

void maze_thread_safe::set_maze(const std::string& maze, unsigned int height) noexcept {
    std::lock_guard<std::mutex> lock(m_maze_mutx);
    this->clear();
    this->m_maze = maze;
    this->m_height = height;
    this->compute_geometry();
};
std::string maze_thread_safe::get_maze() noexcept {
    std::lock_guard<std::mutex> lock(m_maze_mutx);
    return this->m_maze;
}

void maze_thread_safe::clear() noexcept {
    std::lock_guard<std::mutex> lock(m_maze_mutx);
    std::lock_guard<std::shared_mutex> lock_verts(m_verts_mtx);
    m_maze.clear();
    m_wavefront_obj_vertices.clear();
    m_render_vertices.clear();
    m_faces.clear();
}

std::vector<std::tuple<int, int, int, int>> maze_thread_safe::get_render_vertices() noexcept {
    std::shared_lock<std::shared_mutex> lock(m_verts_mtx);
    return this->m_render_vertices;
}

std::vector<std::tuple<int, int, int, int>> maze_thread_safe::get_wavefront_obj_vertices() noexcept {
    std::shared_lock<std::shared_mutex> lock(m_verts_mtx);
    return this->m_wavefront_obj_vertices;
}

std::vector<std::vector<std::uint32_t>> maze_thread_safe::get_faces() noexcept {
    std::shared_lock<std::shared_mutex> lock(m_verts_mtx);
    return this->m_faces;
}

/**
 * @brief Update the maze with a new block, updating geometric write and render vertices
 *  Write vertices are used for Wavefront OBJ file writing
 *  Render vertices are used for rendering in Craft
*/
void maze_thread_safe::add_block(int x, int y, int z, int w, int block_size) noexcept  {
    std::lock_guard<std::shared_mutex> lock(m_verts_mtx);
    // Calculate the base index for the new vertices
    // OBJ format is 1-based indexing
    std::uint32_t baseIndex = static_cast<std::uint32_t>(this->m_wavefront_obj_vertices.size() + 1);
    // Define the 8 vertices of the cube
    this->m_wavefront_obj_vertices.emplace_back(x, y, z, w);
    this->m_wavefront_obj_vertices.emplace_back(x + block_size, y, z, w);
    this->m_wavefront_obj_vertices.emplace_back(x + block_size, y + block_size, z, w);
    this->m_wavefront_obj_vertices.emplace_back(x, y + block_size, z, w);
    this->m_wavefront_obj_vertices.emplace_back(x, y, z + block_size, w);
    this->m_wavefront_obj_vertices.emplace_back(x + block_size, y, z + block_size, w);
    this->m_wavefront_obj_vertices.emplace_back(x + block_size, y + block_size, z + block_size, w);
    this->m_wavefront_obj_vertices.emplace_back(x, y + block_size, z + block_size, w);
    // Render vertices have an implicit block size of "1 unit", useful for 3D rendering
    this->m_render_vertices.emplace_back(x, y, z, w);

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

// Return a future for when maze has been written
std::string maze_thread_safe::to_wavefront_obj_str() const noexcept {
    using namespace std;
    std::lock_guard<std::shared_mutex> lock(m_verts_mtx);
    stringstream ss;
    ss << "# https://www.github.com/zmertens/MazeBuilder\n";

    // keep track of writing progress
    int total_verts = static_cast<int>(m_wavefront_obj_vertices.size());
    int total_faces = static_cast<int>(m_faces.size());

    int t = total_verts + total_faces;
    int c = 0;
    // Write vertices
    for (const auto& vertex : m_wavefront_obj_vertices) {
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

/**
 * @brief Parses the grid, and builds a 3D grid using (x, y, z, w) (w == block type)
 * Specify a "starting_height" to try to put the maze above the heightmap (mountains), and below the clouds
 * @param height of the grid
 * @param grid_as_ascii
 * @return
*/
void maze_thread_safe::compute_geometry() noexcept {
    istringstream iss{ m_maze.data() };
    string line;
    unsigned int row_x = 0;
    while (getline(iss, line, '\n')) {
        unsigned int col_z = 0;
        for (auto itr = line.cbegin(); itr != line.cend() && col_z < line.size(); itr++) {
            if (*itr == '+' || *itr == '-' || *itr == '|') {
                // Check for barriers and walls then iterate up/down
                static constexpr unsigned int starting_height = 30u;
                static constexpr unsigned int block_size = 1;
                auto w{ 5 };
                for (auto h{ starting_height }; h < starting_height + m_height; h++) {
                    // Update the data source that stores the maze geometry
                    // There are 2 data sources, one for rendering and one for writing
                    this->add_block(row_x, h, col_z, w, block_size);
                }
            }
            col_z++;
        }
        row_x++;
    } // getline
} // compute_geometry
