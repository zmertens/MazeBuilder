#include <MazeBuilder/pixels_create_state.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder/output_formats.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/state_utils.h>
#include <MazeBuilder/string_utils.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

using namespace mazes;

namespace
{
    constexpr int cell_size_px = 12;
    constexpr int wall_size_px = 2;
    using rgba_t = std::array<std::uint8_t, 4>;

    std::uint8_t lerp_u8(const std::uint8_t from, const std::uint8_t to, const float t) noexcept
    {
        const float clamped = std::clamp(t, 0.0f, 1.0f);
        return static_cast<std::uint8_t>(static_cast<float>(from) + (static_cast<float>(to) - static_cast<float>(from)) * clamped);
    }

    rgba_t hsv_to_rgba(const float hue, const float saturation, const float value) noexcept
    {
        const float h = std::fmod(hue, 360.0f);
        const float s = std::clamp(saturation, 0.0f, 1.0f);
        const float v = std::clamp(value, 0.0f, 1.0f);

        const float c = v * s;
        const float h_prime = h / 60.0f;
        const float x = c * (1.0f - std::abs(std::fmod(h_prime, 2.0f) - 1.0f));
        const float m = v - c;

        float r1 = 0.0f;
        float g1 = 0.0f;
        float b1 = 0.0f;

        if (h_prime < 1.0f)
        {
            r1 = c;
            g1 = x;
        } else if (h_prime < 2.0f)
        {
            r1 = x;
            g1 = c;
        } else if (h_prime < 3.0f)
        {
            g1 = c;
            b1 = x;
        } else if (h_prime < 4.0f)
        {
            g1 = x;
            b1 = c;
        } else if (h_prime < 5.0f)
        {
            r1 = x;
            b1 = c;
        } else
        {
            r1 = c;
            b1 = x;
        }

        const auto to_byte = [](const float channel) -> std::uint8_t
            {
                const int value_255 = static_cast<int>((channel * 255.0f) + 0.5f);
                return static_cast<std::uint8_t>(std::clamp(value_255, 0, 255));
            };

        return { to_byte(r1 + m), to_byte(g1 + m), to_byte(b1 + m), 255u };
    }
}

pixels_create_state::pixels_create_state(const runtime_app::context& ctx, runtime_stack* stack)
    : write_to_output_state(ctx, stack)
{
}

bool pixels_create_state::write_output(const std::string& output_target, [[maybe_unused]] const std::string& output_content) noexcept
{
    try
    {
        auto& grid_ref = grid_mapper->get(current_grid_id);
        const auto pixels = grid_ref.operations().get_pixels();

        if (!pixels.empty() && m_image_width > 0 && m_image_height > 0)
        {
            const auto out = to_output_format_from_sv(string_utils::file_extension(output_target));
            switch (out)
            {
            case output_format::PNG: return io_utils::write_png(output_target, pixels, m_image_width, m_image_height);
            case output_format::JPG: return io_utils::write_jpg(output_target, pixels, m_image_width, m_image_height);
            default: return io_utils::write_bmp(output_target, pixels, m_image_width, m_image_height);
            };
        }
    } catch (...)
    {
    }

    return true;
}

std::string_view pixels_create_state::create(const configurator& config,
    randomizer& rng) noexcept
{
    if (!grid_mapper)
    {
        return {};
    }

    try
    {
        m_result = "Maze generated";

        auto& grid_ref = grid_mapper->get(current_grid_id);
        auto& grid_ops = grid_ref.operations();

        m_image_width = static_cast<int>(config.columns()) * cell_size_px + (static_cast<int>(config.columns()) + 1) * wall_size_px;
        m_image_height = static_cast<int>(config.rows()) * cell_size_px + (static_cast<int>(config.rows()) + 1) * wall_size_px;

        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(m_image_width) * static_cast<std::size_t>(m_image_height) * 4u, 0u);

        auto set_pixel = [&](const int x, const int y, const rgba_t& rgba)
            {
                const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(m_image_width) + static_cast<std::size_t>(x)) * 4u;
                pixels[offset + 0u] = rgba[0];
                pixels[offset + 1u] = rgba[1];
                pixels[offset + 2u] = rgba[2];
                pixels[offset + 3u] = rgba[3];
            };

        auto fill_rect = [&](const int x0, const int y0, const int width, const int height, const rgba_t& rgba)
            {
                for (int y = y0; y < y0 + height; ++y)
                {
                    for (int x = x0; x < x0 + width; ++x)
                    {
                        set_pixel(x, y, rgba);
                    }
                }
            };

        randomizer deterministic_palette_rng{};
        randomizer* palette_rng = &rng;
        if (m_palette_seed.has_value())
        {
            deterministic_palette_rng.seed(*m_palette_seed);
            palette_rng = &deterministic_palette_rng;
        }

        const float base_hue = static_cast<float>((*palette_rng)(0, 359));
        const rgba_t wall_color = hsv_to_rgba(base_hue,
            static_cast<float>((*palette_rng)(58, 92)) / 100.0f,
            static_cast<float>((*palette_rng)(28, 62)) / 100.0f);
        const rgba_t base_floor_color = hsv_to_rgba(std::fmod(base_hue + static_cast<float>((*palette_rng)(70, 170)), 360.0f),
            static_cast<float>((*palette_rng)(8, 28)) / 100.0f,
            static_cast<float>((*palette_rng)(88, 98)) / 100.0f);
        const rgba_t distance_near_color = hsv_to_rgba(std::fmod(base_hue + static_cast<float>((*palette_rng)(10, 60)), 360.0f),
            static_cast<float>((*palette_rng)(30, 60)) / 100.0f,
            static_cast<float>((*palette_rng)(92, 100)) / 100.0f);
        const rgba_t distance_far_color = hsv_to_rgba(std::fmod(base_hue + static_cast<float>((*palette_rng)(180, 260)), 360.0f),
            static_cast<float>((*palette_rng)(45, 78)) / 100.0f,
            static_cast<float>((*palette_rng)(55, 80)) / 100.0f);

        for (int y = 0; y < m_image_height; ++y)
        {
            for (int x = 0; x < m_image_width; ++x)
            {
                set_pixel(x, y, wall_color);
            }
        }

        const auto* distance_grid_ref = dynamic_cast<const distance_grid*>(&grid_ref);
        const auto distance_map = distance_grid_ref ? distance_grid_ref->get_distances() : std::shared_ptr<distances>{};
        const auto max_distance = distance_map ? distance_map->max().second : 0;

        for (unsigned int row = 0; row < config.rows(); ++row)
        {
            for (unsigned int col = 0; col < config.columns(); ++col)
            {
                const auto index = static_cast<int>(row * config.columns() + col);
                const auto current = grid_ops.search(index);
                if (!current)
                {
                    continue;
                }

                rgba_t floor_color = base_floor_color;
                if (distance_map && distance_map->contains(index))
                {
                    const float t = max_distance > 0 ? static_cast<float>((*distance_map)[index]) / static_cast<float>(max_distance) : 0.0f;
                    floor_color = {
                        lerp_u8(distance_near_color[0], distance_far_color[0], t),
                        lerp_u8(distance_near_color[1], distance_far_color[1], t),
                        lerp_u8(distance_near_color[2], distance_far_color[2], t),
                        255u };
                }

                const int px = wall_size_px + static_cast<int>(col) * (cell_size_px + wall_size_px);
                const int py = wall_size_px + static_cast<int>(row) * (cell_size_px + wall_size_px);

                fill_rect(px, py, cell_size_px, cell_size_px, floor_color);

                if (const auto east = grid_ops.get_east(current); east && current->is_linked(east))
                {
                    fill_rect(px + cell_size_px, py, wall_size_px, cell_size_px, floor_color);
                }

                if (const auto south = grid_ops.get_south(current); south && current->is_linked(south))
                {
                    fill_rect(px, py + cell_size_px, cell_size_px, wall_size_px, floor_color);
                }
            }
        }

        grid_ops.set_pixels(pixels);

        global_async_logger().log(m_result);

        return m_result;
    } catch (...)
    {
        return {};
    }
}
