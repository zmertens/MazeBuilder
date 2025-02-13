#include <MazeBuilder/computations.h>

#include <MazeBuilder/progress.h>
#include <MazeBuilder/maze.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/factory.h>

#include <sstream>
#include <random>

using namespace mazes;

/// <summary>
/// 
/// </summary>
/// <param name="m"></param>
/// <param name=""></param>
void mazes::computations::compute_geometry(const mazes::maze_ptr& m, grid_ptr g) noexcept {
    using namespace std;

    auto add_block = [&m](int x, int y, int z, int w, int block_size) {
        // Calculate the base index for the new vertices
        // OBJ format is 1-based indexing
        // std::uint32_t baseIndex = static_cast<std::uint32_t>(m->vertices.size() + 1);
        // // Define the 8 vertices of the cube
        // m->vertices.emplace_back(x, y, z, w);
        // m->vertices.emplace_back(x + block_size, y, z, w);
        // m->vertices.emplace_back(x + block_size, y + block_size, z, w);
        // m->vertices.emplace_back(x, y + block_size, z, w);
        // m->vertices.emplace_back(x, y, z + block_size, w);
        // m->vertices.emplace_back(x + block_size, y, z + block_size, w);
        // m->vertices.emplace_back(x + block_size, y + block_size, z + block_size, w);
        // m->vertices.emplace_back(x, y + block_size, z + block_size, w);

        // // Define faces using the vertices above (12 triangles for 6 faces)
        // // Front face
        // m->faces.push_back({ {baseIndex, baseIndex + 1, baseIndex + 2} });
        // m->faces.push_back({ {baseIndex, baseIndex + 2, baseIndex + 3} });
        // // Back face
        // m->faces.push_back({ {baseIndex + 4, baseIndex + 6, baseIndex + 5} });
        // m->faces.push_back({ {baseIndex + 4, baseIndex + 7, baseIndex + 6} });
        // // Left face
        // m->faces.push_back({ {baseIndex, baseIndex + 3, baseIndex + 7} });
        // m->faces.push_back({ {baseIndex, baseIndex + 7, baseIndex + 4} });
        // // Right face
        // m->faces.push_back({ {baseIndex + 1, baseIndex + 5, baseIndex + 6} });
        // m->faces.push_back({ {baseIndex + 1, baseIndex + 6, baseIndex + 2} });
        // // Top face
        // m->faces.push_back({ {baseIndex + 3, baseIndex + 2, baseIndex + 6} });
        // m->faces.push_back({ {baseIndex + 3, baseIndex + 6, baseIndex + 7} });
        // // Bottom face
        // m->faces.push_back({ {baseIndex, baseIndex + 4, baseIndex + 5} });
        // m->faces.push_back({ {baseIndex, baseIndex + 5, baseIndex + 1} });
    
    }; // lambda

    // mt19937 mt(m->seed);
    // auto get_int = [&mt](int min, int max) {
    //     uniform_int_distribution<int> dist(min, max);
    //     return dist(mt);
    // };

    // // Generate maze and time it
    // progress p{};
    // bool use_get_int = (m->block_type == -1) ? true : false;
    // istringstream iss{ m->to_str() };
    // string line;
    // int row_x = 0;
    // while (getline(iss, line, '\n')) {
    //     int col_z = 0;
    //     for (auto itr = line.cbegin(); itr != line.cend() && col_z < line.size(); itr++) {
    //         // Check for barriers and walls then iterate up to the height of the maze
    //         if (*itr == MAZE_CORNER || *itr == MAZE_BARRIER1 || *itr == MAZE_BARRIER2) {
    //             static constexpr int block_size = 1;
    //             for (auto h{ 0 }; h < m->height; h++) {
    //                 // Update the data source that stores the maze geometry
    //                 // There are 2 data sources, one for rendering and one for writing
    //                 int block_type = (use_get_int) ? get_int(3, 14) : m->block_type;
    //                 add_block(row_x, h, col_z, block_type, block_size);
    //                 m->intopq(row_x + m->offset_x, h, col_z + m->offset_z, block_type);
    //             }
    //         }
    //         col_z++;
    //     }
    //     row_x++;
    // } // getline
    // auto elapsed = p.elapsed_ms();
    // p.reset();
} // compute_geometry

std::string computations::stringify(const std::unique_ptr<maze>& p) noexcept {
    using namespace std;
    ostringstream oss;
    if (p) {
        oss << *(p->get_grid());
    } else {
        oss << "INFO: Grid pointer is null";
    }
    return oss.str();
} // stringify
