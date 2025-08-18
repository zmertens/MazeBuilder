#include <MazeBuilder/objectify.h>

#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/enums.h>

#include <tuple>
#include <vector>
#include <cstdint>
#include <string_view>

using namespace mazes;

/// @brief Generate 3D mesh data from the grid for Wavefront object file output
/// @param g The grid interface
/// @param rng Random number generator (unused but required by interface)
/// @return True if successful, false otherwise
bool objectify::run(std::unique_ptr<grid_interface> const& g, randomizer& rng) const noexcept {

    using namespace std;

    if (!g) {
        // Handle null grid
        return false;
    }

    // Get the grid operations to access dimensions and other methods
    const auto& grid_ops = g->operations();
    auto dimensions = grid_ops.get_dimensions();
    if (get<0>(dimensions) == 0 || get<1>(dimensions) == 0 || get<2>(dimensions) == 0) {
        // Handle invalid dimensions
        return false;
    }

    // Get the string representation of the maze
    std::string str = grid_ops.get_str();
    if (str.empty()) {
        return false;
    }

    // Prepare data structures for vertices and faces
    std::vector<std::tuple<int, int, int, int>> vertices;
    std::vector<std::vector<std::uint32_t>> faces;

    // Lambda to add a block (cube) at given position
    auto add_block = [&vertices, &faces](int x, int y, int z, int w, int block_size) {
        // Calculate the base index for the new vertices (OBJ format is 1-indexed)
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

        // Define faces using the vertices above (12 triangles for 6 faces of a cube)
        // Bottom face (z=0)
        faces.emplace_back(std::vector<std::uint32_t>{baseIndex, baseIndex + 1, baseIndex + 2});
        faces.emplace_back(std::vector<std::uint32_t>{baseIndex, baseIndex + 2, baseIndex + 3});
        // Top face (z=block_size)
        faces.emplace_back(std::vector<std::uint32_t>{baseIndex + 4, baseIndex + 6, baseIndex + 5});
        faces.emplace_back(std::vector<std::uint32_t>{baseIndex + 4, baseIndex + 7, baseIndex + 6});
        // Left face (x=0)
        faces.emplace_back(std::vector<std::uint32_t>{baseIndex, baseIndex + 3, baseIndex + 7});
        faces.emplace_back(std::vector<std::uint32_t>{baseIndex, baseIndex + 7, baseIndex + 4});
        // Right face (x=block_size)
        faces.emplace_back(std::vector<std::uint32_t>{baseIndex + 1, baseIndex + 5, baseIndex + 6});
        faces.emplace_back(std::vector<std::uint32_t>{baseIndex + 1, baseIndex + 6, baseIndex + 2});
        // Front face (y=block_size)
        faces.emplace_back(std::vector<std::uint32_t>{baseIndex + 3, baseIndex + 2, baseIndex + 6});
        faces.emplace_back(std::vector<std::uint32_t>{baseIndex + 3, baseIndex + 6, baseIndex + 7});
        // Back face (y=0)
        faces.emplace_back(std::vector<std::uint32_t>{baseIndex, baseIndex + 4, baseIndex + 5});
        faces.emplace_back(std::vector<std::uint32_t>{baseIndex, baseIndex + 5, baseIndex + 1});
    };

    // Parse the string representation and create 3D blocks for walls
    int row_x = 0;
    int col_z = 0;
    
    std::string_view sv(str);
    for (size_t i = 0; i < sv.size(); ++i) {
        if (sv[i] == '\n') {
            row_x++;
            col_z = 0;
            continue;
        }

        // Create blocks for wall characters
        if (sv[i] == CORNER || sv[i] == BARRIER1 || sv[i] == BARRIER2) {
            static constexpr auto block_size = 1;
            for (auto h = 0; h < static_cast<int>(std::get<2>(dimensions)); ++h) {
                add_block(row_x, col_z, h, 0, block_size); // w=0 for now, could use block_id from config
            }
        }
        col_z++;
    }

    // Store the generated vertices and faces in the grid operations
    // Need to cast to non-const to call the setter methods
    auto& mutable_grid_ops = const_cast<grid_operations&>(grid_ops);
    mutable_grid_ops.set_vertices(vertices);
    mutable_grid_ops.set_faces(faces);

    return true;
} // run
