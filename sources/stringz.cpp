#include <MazeBuilder/stringz.h>

#include <MazeBuilder/maze.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid_interface.h>

#if defined(MAZE_DEBUG)
#include <sstream>
#include <iostream>
#endif

#include <random>
#include <tuple>
#include <unordered_set>

using namespace mazes;

/// @brief 
/// @param m 
/// @param vertices 
/// @param faces
/// @param s {}
/// Benchmark => 3s avg
void stringz::objectify(const std::unique_ptr<maze>& m,
    std::vector<std::tuple<int, int, int, int>>& vertices,
    std::vector<std::vector<std::uint32_t>>& faces,
    std::string_view sv) noexcept {
    using namespace std;

    if (!m) {
        // Handle null maze pointer
        return;
    }

    auto dimensions = m->get_grid()->get_dimensions();
    if (get<0>(dimensions) == 0 || get<1>(dimensions) == 0 || get<2>(dimensions) == 0) {
        // Handle invalid dimensions
        return;
    }

    auto add_block = [&vertices, &faces](int x, int y, int z, int w, int block_size) {
        // Calculate the base index for the new vertices
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
        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex, baseIndex + 1, baseIndex + 2});
        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex, baseIndex + 2, baseIndex + 3});
        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex + 4, baseIndex + 6, baseIndex + 5});
        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex + 4, baseIndex + 7, baseIndex + 6});
        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex, baseIndex + 3, baseIndex + 7});
        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex, baseIndex + 7, baseIndex + 4});
        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex + 1, baseIndex + 5, baseIndex + 6});
        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex + 1, baseIndex + 6, baseIndex + 2});
        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex + 3, baseIndex + 2, baseIndex + 6});
        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex + 3, baseIndex + 6, baseIndex + 7});
        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex, baseIndex + 4, baseIndex + 5});
        faces.emplace_back(std::initializer_list<std::uint32_t>{baseIndex, baseIndex + 5, baseIndex + 1});
    };

    int row_x = 0;

#if defined(MAZE_DEBUG)
    istringstream iss{ sv.data()};
    string line = "";
    while (getline(iss, line, '\n')) {
        int col_z = 0;

        for (auto itr = line.cbegin(); itr != line.cend() && col_z < line.size(); itr++) {
            // Check for barriers and walls then iterate up to the height of the maze
            if (*itr == CORNER || *itr == BARRIER1 || *itr == BARRIER2) {
                static constexpr auto block_size = 1;

                for (auto h{ 0 }; h < get<2>(dimensions); h++) {
                    add_block(row_x, col_z, h, m->get_block_id(), block_size);
                }
            }
            col_z++;
        }
        row_x++;
    } // getline
#else
    int col_z = 0;
    for (size_t i = 0; i < sv.size(); ++i) {
        if (sv[i] == '\n') {
            row_x++;
            col_z = 0;
            continue;
        }

        if (sv[i] == CORNER || sv[i] == BARRIER1 || sv[i] == BARRIER2) {
            static constexpr auto block_size = 1;
            for (auto h = 0; h < m->get_levels(); ++h) {
                add_block(row_x, col_z, h, m->get_block_id(), block_size);
            }
        }
        col_z++;
    }
#endif
}

void stringz::objectify(lab& labyrinth, std::string_view sv) noexcept {
    using namespace std;

    int row_x = 0;

#if defined(MAZE_DEBUG)
    istringstream iss{ sv.data() };
    string line = "";
    while (getline(iss, line, '\n')) {
        int col_z = 0;

        for (auto itr = line.cbegin(); itr != line.cend() && col_z < line.size(); itr++) {
            // Check for barriers and walls then iterate up to the height of the maze
            if (*itr == CORNER || *itr == BARRIER1 || *itr == BARRIER2) {
                static constexpr auto block_size = 1;

                for (auto h{ 0 }; h < labyrinth.get_levels(); h++) {
                    labyrinth.insert(row_x, col_z, h, labyrinth.get_random_block_id());
                }
            }
            col_z++;
        }
        row_x++;
    } // getline
#else
    int col_z = 0;
    for (size_t i = 0; i < sv.size(); ++i) {
        if (sv[i] == '\n') {
            row_x++;
            col_z = 0;
            continue;
        }

        if (sv[i] == CORNER || sv[i] == BARRIER1 || sv[i] == BARRIER2) {
            static constexpr auto block_size = 1;
            for (auto h = 0; h < labyrinth.get_levels(); ++h) {
                labyrinth.insert(row_x, col_z, h, labyrinth.get_random_block_id());
            }
        }
        col_z++;
    }
#endif

}

/// @brief Converts a string representation of an image to a pixel array.
/// @param s 
/// @param pixels 
/// @param width 
/// @param height 
/// @param stride 4
void stringz::to_pixels(const std::string& s, std::vector<std::uint8_t>& pixels, int& width, int& height, int stride) noexcept {
    using namespace std;

    auto temp_width{ 0 };
    width = height = 0;

    // Determine width and height (assume fixed dimensions)
    for (char c : s) {
        if (c == '\n') {
            height += 1;
            if (temp_width > width) {
                width = temp_width;
            }
            temp_width = 0;
        } else {
            temp_width += 1;
        }
    }

    // Account for the last line in the file
    if (temp_width > 0) {
        height += 1;
        if (temp_width > width) {
            width = temp_width;
        }
    }

    // Initialize the pixels as 255 (white)
    pixels.resize(width * height * stride, 255);

    // Fill the pixels by parsing the string
    int x{ 0 }, y{ 0 };
    for (char c : s) {
        if (c == '\n') {
            y += 1;
            x = 0;
        } else {
            auto index = (y * width + x) * stride;
            pixels[index] = pixels[index + 1] = pixels[index + 2] = (c == CORNER || c == BARRIER1 || c == BARRIER2) ? 0 : 255;
            // Alpha channel
            pixels[index + 3] = 255;
            x += 1;
        }
    }

#if defined(MAZE_DEBUG)
    cout << "Width: " << width << " Height: " << height << endl;
    cout << "Pixels: " << pixels.size() << endl;
#endif
}

/// @brief Generates a pixel array from a maze object using cell background colors.
/// @param m The maze object containing the grid and cells.
/// @param pixels The output pixel array.
/// @param width The width of the resulting image.
/// @param height The height of the resulting image.
/// @param stride The number of bytes per pixel (e.g., 4 for RGBA).
void stringz::to_pixels(const std::unique_ptr<maze>& m, std::vector<std::uint8_t>& pixels, int& width, int& height, int stride) noexcept {
    using namespace std;

    to_pixels(stringify(cref(m)), ref(pixels), ref(width), ref(height), stride);
}


/// @brief 
/// @param m 
/// @return
/// Benchmark => 300ms avg
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
