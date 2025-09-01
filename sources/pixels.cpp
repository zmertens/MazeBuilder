#include <MazeBuilder/pixels.h>

#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/randomizer.h>

using namespace mazes;

/// @brief Provide a string representation of the grid
/// @param g
/// @param rng
/// @return 
bool pixels::run([[maybe_unused]] grid_interface* g, [[maybe_unused]] randomizer & rng) const noexcept {
    return false;
  /*  void string_view_utils::pixels(const std::unique_ptr<maze>&m,
        std::vector<std::tuple<int, int, int, int>>&vertices,
        std::vector<std::vector<std::uint32_t>>&faces,
        std::string_view sv) noexcept {
        using namespace std;*/

    //    if (!m) {
    //        // Handle null maze pointer
    //        return;
    //    }

    //    auto dimensions = m->get_dimensions();
    //    if (get<0>(dimensions) == 0 || get<1>(dimensions) == 0 || get<2>(dimensions) == 0) {
    //        // Handle invalid dimensions
    //        return;
    //    }

    //    auto add_block = [&vertices, &faces](int x, int y, int z, int w, int block_size) {
    //        // Calculate the base index for the new vertices
    //        std::uint32_t baseIndex = static_cast<std::uint32_t>(vertices.size() + 1);
    //        // Define the 8 vertices of the cube
    //        vertices.emplace_back(x, y, z, w);
    //        vertices.emplace_back(x + block_size, y, z, w);
    //        vertices.emplace_back(x + block_size, y + block_size, z, w);
    //        vertices.emplace_back(x, y + block_size, z, w);
    //        vertices.emplace_back(x, y, z + block_size, w);
    //        vertices.emplace_back(x + block_size, y, z + block_size, w);
    //        vertices.emplace_back(x + block_size, y + block_size, z + block_size, w);
    //        vertices.emplace_back(x, y + block_size, z + block_size, w);

    //        // Define faces using the vertices above (12 triangles for 6 faces)
    //        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex, baseIndex + 1, baseIndex + 2});
    //        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex, baseIndex + 2, baseIndex + 3});
    //        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex + 4, baseIndex + 6, baseIndex + 5});
    //        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex + 4, baseIndex + 7, baseIndex + 6});
    //        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex, baseIndex + 3, baseIndex + 7});
    //        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex, baseIndex + 7, baseIndex + 4});
    //        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex + 1, baseIndex + 5, baseIndex + 6});
    //        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex + 1, baseIndex + 6, baseIndex + 2});
    //        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex + 3, baseIndex + 2, baseIndex + 6});
    //        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex + 3, baseIndex + 6, baseIndex + 7});
    //        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex, baseIndex + 4, baseIndex + 5});
    //        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex, baseIndex + 5, baseIndex + 1});
    //        };

    //    int row_x = 0;

    //    int col_z = 0;
    //    for (size_t i = 0; i < sv.size(); ++i) {
    //        if (sv[i] == '\n') {
    //            row_x++;
    //            col_z = 0;
    //            continue;
    //        }

    //        if (sv[i] == CORNER || sv[i] == BARRIER1 || sv[i] == BARRIER2) {
    //            static constexpr auto block_size = 1;
    //            for (auto h = 0; h < std::get<2>(dimensions); ++h) {
    //                add_block(row_x, col_z, h, m->get_block_id(), block_size);
    //            }
    //        }
    //        col_z++;
    //    }
    //}
} // run
