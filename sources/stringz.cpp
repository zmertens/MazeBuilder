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

/// @brief Generates a pixel array from a maze object.
/// @param m 
/// @param pixels 
/// @param width
/// @param height
/// @param stride 4
void stringz::to_pixels(const std::unique_ptr<maze>& m, std::vector<std::uint8_t>& pixels, int& width, int& height, int stride) noexcept {
    using namespace std;

    vector<vector<shared_ptr<cell>>> cells2;
    m->get_grid()->to_vec2(ref(cells2));

    auto dimensions = m->get_grid()->get_dimensions();
    width = get<0>(dimensions) * 2 + 1;
    height = get<1>(dimensions) * 2 + 1;

    // Initialize the pixels as 255 (white)
    pixels.resize(width * height * stride, 255);
    // Ensure cells2 is correctly sized
    if (cells2.size() != get<0>(dimensions) || (cells2.size() > 0 && cells2[0].size() != get<1>(dimensions))) {
        // Handle incorrect sizing
        return;
    }

    // Fill the pixels using the cells and their gradient colors
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int cell_y = y / 2;
            int cell_x = x / 2;

            // Check if the indices are within bounds
            if (cell_y >= cells2.size() || cell_x >= cells2[cell_y].size()) {

                continue;
            }

            auto c = cells2[cell_y][cell_x];

            if (!c) {

                continue;
            }

            auto color_opt = m->get_grid()->background_color_for(cref(c));
            uint32_t color = color_opt.value_or(0xFFFFFF);

            auto index = (y * width + x) * stride;
            // Red
            pixels[index] = (color >> 16) & 0xFF;
            // Green
            pixels[index + 1] = (color >> 8) & 0xFF;
            // Blue
            pixels[index + 2] = color & 0xFF;
            // Alpha channel
            pixels[index + 3] = 255;
        }
    }

    // Apply wall coloring
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int cell_y = y / 2;
            int cell_x = x / 2;

            // Check if the indices are within bounds
            if (cell_y >= cells2.size() || cell_x >= cells2[cell_y].size()) {

                continue;
            }

            auto c = cells2[cell_y][cell_x];

            if (!c) {
                continue;
            }

            uint32_t wall_color = 0x000000;

            // Set the walls 
            if (c->has_northern_neighbor() && !c->is_linked(c->get_north()) && y % 2 == 0) {
                auto index = (y * width + x) * stride;
                pixels[index] = (wall_color >> 16) & 0xFF;
                pixels[index + 1] = (wall_color >> 8) & 0xFF;
                pixels[index + 2] = wall_color & 0xFF;
                pixels[index + 3] = 255;
            }
            if (c->has_southern_neighbor() && !c->is_linked(c->get_south()) && y % 2 == 1) {
                auto index = (y * width + x) * stride;
                pixels[index] = (wall_color >> 16) & 0xFF;
                pixels[index + 1] = (wall_color >> 8) & 0xFF;
                pixels[index + 2] = wall_color & 0xFF;
                pixels[index + 3] = 255;
            }
            if (c->has_eastern_neighbor() && !c->is_linked(c->get_east()) && x % 2 == 1) {
                auto index = (y * width + x) * stride;
                pixels[index] = (wall_color >> 16) & 0xFF;
                pixels[index + 1] = (wall_color >> 8) & 0xFF;
                pixels[index + 2] = wall_color & 0xFF;
                pixels[index + 3] = 255;
            }
            if (c->has_western_neighbor() && !c->is_linked(c->get_west()) && x % 2 == 0) {
                auto index = (y * width + x) * stride;
                pixels[index] = (wall_color >> 16) & 0xFF;
                pixels[index + 1] = (wall_color >> 8) & 0xFF;
                pixels[index + 2] = wall_color & 0xFF;
                pixels[index + 3] = 255;
            }
        }
    }

#if defined(MAZE_DEBUG)
    cout << "Width: " << width << "\nHeight: " << height << endl;
    cout << "Stride: " << stride << endl;
    cout << "Pixels: " << pixels.size() << endl;
#endif
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
