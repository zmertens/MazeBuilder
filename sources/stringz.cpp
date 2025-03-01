#include <MazeBuilder/stringz.h>

#include <MazeBuilder/maze.h>
#include <MazeBuilder/enums.h>

#include <sstream>
#include <random>

using namespace mazes;

/// @brief 
/// @param m 
/// @param vertices 
/// @param faces 
void stringz::objectify(const std::unique_ptr<maze>& m,
    std::vector<std::tuple<int, int, int, int>>& vertices,
    std::vector<std::vector<std::uint32_t>>& faces) noexcept {
    using namespace std;

    auto add_block = [&vertices, &faces](int x, int y, int z, int w, int block_size) {
        // Calculate the base index for the new vertices
        // OBJ format is 1-based indexing
        std::uint32_t baseIndex = static_cast<std::uint32_t>(vertices.size() + 1);
        // Define the 8 vertices of the cube
        vertices.emplace_back(x, y, z, w);
        vertices.emplace_back(x + block_size, y, z, w);
        vertices.emplace_back(x + block_size, y + block_size, z, w);
        vertices.emplace_back(x, y + block_size, z, w);
        vertices.emplace_back(x, y, z + block_size, w);
        vertices.emplace_back(x + block_size, y, z + block_size, w);
        vertices.emplace_back(x + block_size, y + block_size, z + block_size, w);
        vertices.emplace_back(x, y + block_size, z + block_size, w);

        // Define faces using the vertices above (12 triangles for 6 faces)
        // Front face
        faces.push_back({ {baseIndex, baseIndex + 1, baseIndex + 2} });
        faces.push_back({ {baseIndex, baseIndex + 2, baseIndex + 3} });
        // Back face
        faces.push_back({ {baseIndex + 4, baseIndex + 6, baseIndex + 5} });
        faces.push_back({ {baseIndex + 4, baseIndex + 7, baseIndex + 6} });
        // Left face
        faces.push_back({ {baseIndex, baseIndex + 3, baseIndex + 7} });
        faces.push_back({ {baseIndex, baseIndex + 7, baseIndex + 4} });
        // Right face
        faces.push_back({ {baseIndex + 1, baseIndex + 5, baseIndex + 6} });
        faces.push_back({ {baseIndex + 1, baseIndex + 6, baseIndex + 2} });
        // Top face
        faces.push_back({ {baseIndex + 3, baseIndex + 2, baseIndex + 6} });
        faces.push_back({ {baseIndex + 3, baseIndex + 6, baseIndex + 7} });
        // Bottom face
        faces.push_back({ {baseIndex, baseIndex + 4, baseIndex + 5} });
        faces.push_back({ {baseIndex, baseIndex + 5, baseIndex + 1} });
    
    }; // lambda

    istringstream iss{ stringify(cref(m)) };

    string line = "";

    int row_x = 0;

    while (getline(iss, line, '\n')) {
        int col_z = 0;

        for (auto itr = line.cbegin(); itr != line.cend() && col_z < line.size(); itr++) {

            // Check for barriers and walls then iterate up to the height of the maze
            if (*itr == CORNER || *itr == BARRIER1 || *itr == BARRIER2) {
                static constexpr auto block_size = 1;

                for (auto h{ 0 }; h < m->get_levels(); h++) {

                    add_block(row_x, h, col_z, m->get_block_id(), block_size);
                }
            }
            col_z++;
        }
        row_x++;
    } // getline

} // objectify

std::string stringz::stringify(const std::unique_ptr<maze>& m) noexcept {
    using namespace std;

    ostringstream oss;
    if (m) {
        oss << *(m->get_grid());
    } else {
        oss << "Maze pointer is null";
    }
    return oss.str();
} // stringify
