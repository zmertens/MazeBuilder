#include <MazeBuilder/pixels_create_state.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_stack.h>

using namespace mazes;

pixels_create_state::pixels_create_state(const runtime_app::context &ctx, runtime_stack *stack)
: state(ctx, stack), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}
{

}

void pixels_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool pixels_create_state::update([[maybe_unused]] const std::optional<args> &args, [[maybe_unused]] double delta_time) noexcept
{
    // Implementation of the update function
    return true;
}


std::string_view pixels_create_state::create(algo a, unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept
{
    if (a != algo::PIXELS)
    {
        return std::string_view{"Unsupported algorithm"};
    }

    auto sum = rows + cols + levels;
    [[maybe_unused]] auto sum1 = rng(0, sum);
    return std::string_view{"Pixels maze creation not yet implemented"};
}

    //  if (!g)
    //     {
    //         return false;
    //     }

    //     // Get the grid operations to access dimensions and other methods
    //     auto &grid_ops = g->operations();

    //     // Ensure we have a string representation first
    //     std::string maze_str = grid_ops.get_str();
    //     if (maze_str.empty())
    //     {
    //         // Run stringify if not already done
    //         if (stringify stringifier; !stringifier.run(g, rng))
    //         {
    //             return false;
    //         }
    //         maze_str = grid_ops.get_str();

    //         if (maze_str.empty())
    //         {
    //             return false;
    //         }
    //     }

    //     // Calculate scale based on grid dimensions
    //     auto [rows, columns, _] = grid_ops.get_dimensions();

    //     constexpr unsigned int MIN_SCALE = 1;
    //     constexpr unsigned int MAX_SCALE = 10;

    //     auto calculated_scale = static_cast<unsigned int>(std::sqrt(static_cast<double>(rows * columns)));

    //     auto scale = std::clamp(calculated_scale, MIN_SCALE, MAX_SCALE);

    //     // Parse the ASCII string to determine dimensions
    //     std::istringstream iss(maze_str);
    //     std::string line;
    //     std::vector<std::string> lines;

    //     while (std::getline(iss, line))
    //     {
    //         lines.push_back(line);
    //     }

    //     if (lines.empty())
    //     {
    //         return false;
    //     }

    //     // Calculate pixel dimensions
    //     const size_t ascii_height = lines.size();

    //     // Find the maximum line length to handle lines with trimmed trailing spaces
    //     size_t ascii_width = 0;
    //     size_t min_line_length = SIZE_MAX;
    //     for (const auto & l : lines)
    //     {
    //         ascii_width = std::max(ascii_width, l.length());
    //         min_line_length = std::min(min_line_length, l.length());
    //     }

    //     const auto pixel_width = static_cast<unsigned int>(ascii_width * scale);
    //     const auto pixel_height = static_cast<unsigned int>(ascii_height * scale);

    //     // RGBA format - 4 bytes per pixel
    //     constexpr unsigned int STRIDE = 4;
    //     const size_t pixel_data_size = static_cast<size_t>(pixel_width) * pixel_height * STRIDE;

    //     std::vector<std::uint8_t> pixel_data(pixel_data_size);

    //     // Convert ASCII to pixels
    //     for (size_t ascii_y = 0; ascii_y < ascii_height; ++ascii_y)
    //     {
    //         const std::string &current_line = lines[ascii_y];

    //         for (size_t ascii_x = 0; ascii_x < ascii_width; ++ascii_x)
    //         {
    //             // Define colors (RGBA format)
    //             constexpr std::uint8_t BLACK_R = 0x00;
    //             constexpr std::uint8_t BLACK_G = 0x00;
    //             constexpr std::uint8_t BLACK_B = 0x00;
    //             constexpr std::uint8_t BLACK_A = 0xFF;

    //             constexpr std::uint8_t WHITE_R = 0xFF;
    //             constexpr std::uint8_t WHITE_G = 0xFF;
    //             constexpr std::uint8_t WHITE_B = 0xFF;
    //             constexpr std::uint8_t WHITE_A = 0xFF;
    //             // Get character at position, treating missing/beyond-length as space (passage)
    //             const char ch = (ascii_x < current_line.length()) ? current_line[ascii_x] : ' ';

    //             // Determine if this is a wall or passage
    //             const bool is_wall = ch == static_cast<unsigned char>(barriers::CORNER) ||
    //                 ch == static_cast<unsigned char>(barriers::HORIZONTAL) ||
    //                 ch == static_cast<unsigned char>(barriers::VERTICAL);

    //             const std::uint8_t red = is_wall ? BLACK_R : WHITE_R;
    //             const std::uint8_t green = is_wall ? BLACK_G : WHITE_G;
    //             const std::uint8_t blue = is_wall ? BLACK_B : WHITE_B;
    //             const std::uint8_t alpha = is_wall ? BLACK_A : WHITE_A;

    //             // Fill scaled pixel block
    //             for (unsigned int sy = 0; sy < scale; ++sy)
    //             {
    //                 for (unsigned int sx = 0; sx < scale; ++sx)
    //                 {
    //                     const size_t pixel_y = ascii_y * scale + sy;

    //                     if (const size_t pixel_x = ascii_x * scale + sx; pixel_y < pixel_height && pixel_x < pixel_width)
    //                     {
    //                         if (const size_t pixel_index = (pixel_y * pixel_width + pixel_x) * STRIDE;
    //                             pixel_index + 3 < pixel_data_size)
    //                         {
    //                             pixel_data[pixel_index + 0] = red;
    //                             pixel_data[pixel_index + 1] = green;
    //                             pixel_data[pixel_index + 2] = blue;
    //                             pixel_data[pixel_index + 3] = alpha;
    //                         }
    //                     }
    //                 }
    //             }
    //         }
    //     }

    //     // Store the pixel data in the grid
    //     grid_ops.set_pixels(pixel_data);

    //     return true;
