#include <MazeBuilder/pixels_create_state.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/maze_state_utils.h>
#include <MazeBuilder/output_formats.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_stack.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace mazes;

namespace
{
    constexpr int cell_size_px = 12;
    constexpr int wall_size_px = 2;

    std::uint8_t lerp_u8(const std::uint8_t from, const std::uint8_t to, const float t) noexcept
    {
        const float clamped = std::clamp(t, 0.0f, 1.0f);
        return static_cast<std::uint8_t>(static_cast<float>(from) + (static_cast<float>(to) - static_cast<float>(from)) * clamped);
    }
}

pixels_create_state::pixels_create_state(const runtime_app::context &ctx, runtime_stack *stack)
    : state(ctx, stack), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}
{
}

void pixels_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool pixels_create_state::update([[maybe_unused]] const std::optional<args> &args,
                                 [[maybe_unused]] double delta_time) noexcept
{
    m_result.clear();

    if (!args.has_value() || !grid_mapper || !processed_text_mapper)
    {
        request_stack_pop();
        return false;
    }

    unsigned int rows = configurator::MAX_ROWS;
    unsigned int cols = configurator::MAX_COLUMNS;
    unsigned int levels = 1u;
    maze_state_utils::parse_dimensions(args, rows, cols, levels);

    m_grid_id = maze_state_utils::has_distances(args) ? grid_identifier::DISTANCE : grid_identifier::BASIC;

    std::string output_target;
    if (const auto parsed = args->get(); parsed.has_value())
    {
        if (const auto it = parsed->find(mazes::args::OUTPUT_ID_WORD_STR); it != parsed->cend())
        {
            output_target = it->second;
        }
    }

    randomizer fallback_rng{};
    auto *rng_ptr = maze_state_utils::get_rng_or_default(get_context(), fallback_rng);

    configurator cfg{};
    cfg.ensure_rows(rows)
        .ensure_columns(cols)
        .ensure_levels(levels)
        .ensure_distances(m_grid_id == grid_identifier::DISTANCE)
        .ensure_algo_id(algo::PIXELS);

    m_result = std::string{create(cfg, *rng_ptr)};

    if (!m_result.empty() && !output_target.empty())
    {
        bool write_ok = false;

        try
        {
            auto &grid_ref = grid_mapper->get(m_grid_id);
            const auto pixels = grid_ref.operations().get_pixels();
            const auto extension = std::filesystem::path{output_target}.extension().string();

            std::string normalized = extension;
            std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch)
                           { return static_cast<char>(std::tolower(ch)); });

            if (!pixels.empty() && m_image_width > 0 && m_image_height > 0)
            {
                if (normalized == ".png")
                {
                    write_ok = stbi_write_png(output_target.c_str(), m_image_width, m_image_height, 4, pixels.data(), m_image_width * 4) != 0;
                }
                else if (normalized == ".bmp")
                {
                    write_ok = stbi_write_bmp(output_target.c_str(), m_image_width, m_image_height, 4, pixels.data()) != 0;
                }
                else if (normalized == ".jpg" || normalized == ".jpeg")
                {
                    write_ok = stbi_write_jpg(output_target.c_str(), m_image_width, m_image_height, 4, pixels.data(), 95) != 0;
                }
            }
        }
        catch (...)
        {
        }

        m_result = write_ok ? "Wrote maze to " + output_target : "Failed to write maze to " + output_target;
    }

    if (processed_text_mapper)
    {
        try
        {
            auto &finished = processed_text_mapper->get(processed_text_identifier::FINISHED);
            finished.set_dirty(m_result);
            finished.set_processed(m_result);
        }
        catch (...)
        {
        }
    }

    request_stack_pop();
    return false;
}

std::string_view pixels_create_state::create(const configurator &config,
                                             randomizer &rng) noexcept
{
    if (config.algo_id() != algo::PIXELS || !grid_mapper)
    {
        return {};
    }

    try
    {
        auto &grid_ref = grid_mapper->get(m_grid_id);
        auto &grid_ops = grid_ref.operations();

        m_image_width = static_cast<int>(config.columns()) * cell_size_px + (static_cast<int>(config.columns()) + 1) * wall_size_px;
        m_image_height = static_cast<int>(config.rows()) * cell_size_px + (static_cast<int>(config.rows()) + 1) * wall_size_px;

        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(m_image_width) * static_cast<std::size_t>(m_image_height) * 4u, 0u);

        auto set_pixel = [&](const int x, const int y, const std::array<std::uint8_t, 4> &rgba)
        {
            if (x < 0 || y < 0 || x >= m_image_width || y >= m_image_height)
            {
                return;
            }

            const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(m_image_width) + static_cast<std::size_t>(x)) * 4u;
            pixels[offset + 0u] = rgba[0];
            pixels[offset + 1u] = rgba[1];
            pixels[offset + 2u] = rgba[2];
            pixels[offset + 3u] = rgba[3];
        };

        auto fill_rect = [&](const int x0, const int y0, const int width, const int height, const std::array<std::uint8_t, 4> &rgba)
        {
            for (int y = y0; y < y0 + height; ++y)
            {
                for (int x = x0; x < x0 + width; ++x)
                {
                    set_pixel(x, y, rgba);
                }
            }
        };

        const std::array<std::uint8_t, 4> wall_color{24u, 28u, 34u, 255u};
        const std::array<std::uint8_t, 4> base_floor_color{244u, 241u, 232u, 255u};

        for (int y = 0; y < m_image_height; ++y)
        {
            for (int x = 0; x < m_image_width; ++x)
            {
                set_pixel(x, y, wall_color);
            }
        }

        const auto *distance_grid_ref = dynamic_cast<const distance_grid *>(&grid_ref);
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

                std::array<std::uint8_t, 4> floor_color = base_floor_color;
                if (distance_map && distance_map->contains(index))
                {
                    const float t = max_distance > 0 ? static_cast<float>((*distance_map)[index]) / static_cast<float>(max_distance) : 0.0f;
                    floor_color = {
                        lerp_u8(255u, 70u, t),
                        lerp_u8(238u, 120u, t),
                        lerp_u8(179u, 245u, t),
                        255u};
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
        [[maybe_unused]] auto noise = rng(0, 1);
        m_result = "Maze image generated";
        return m_result;
    }
    catch (...)
    {
        return {};
    }
}
