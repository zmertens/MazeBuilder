#include <MazeBuilder/wavefront_object_create_state.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/state_utils.h>

#include <sstream>
#include <tuple>
#include <vector>

#include <fmt/format.h>

using namespace mazes;

wavefront_object_create_state::wavefront_object_create_state(const runtime_app::context &ctx, runtime_stack *rs)
    : state(ctx, rs), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}
    , current_grid_id{grid_identifier::BASIC}, m_result{}
{
    state_utils::validate_mappers(grid_mapper, processed_text_mapper);
}

std::string_view wavefront_object_create_state::create(const configurator &config, randomizer &rng) noexcept
{
    try
    {
        m_result.clear();

        auto &grid_ref = grid_mapper->get(current_grid_id);
        auto &grid_ops = grid_ref.operations();

        constexpr float WALL_HEIGHT = 1.0f;
        constexpr float LEVEL_GAP = 2.0f;
        float wall_thickness = rng.get_float(0.05f, 0.1f);

        std::vector<std::array<float, 3>> vertices;
        std::vector<std::array<unsigned int, 4>> faces;

        auto append_box = [&](float x0, float y0, float z0, float x1, float y1, float z1)
        {
            // OBJ indices are 1-based, not 0-based.
            const auto base = static_cast<unsigned int>(vertices.size()) + 1u;

            vertices.push_back({x0, y0, z0});
            vertices.push_back({x1, y0, z0});
            vertices.push_back({x1, y1, z0});
            vertices.push_back({x0, y1, z0});
            vertices.push_back({x0, y0, z1});
            vertices.push_back({x1, y0, z1});
            vertices.push_back({x1, y1, z1});
            vertices.push_back({x0, y1, z1});

            faces.push_back({base + 4u, base + 5u, base + 6u, base + 7u});
            faces.push_back({base + 1u, base + 0u, base + 3u, base + 2u});
            faces.push_back({base + 3u, base + 7u, base + 6u, base + 2u});
            faces.push_back({base + 0u, base + 1u, base + 5u, base + 4u});
            faces.push_back({base + 1u, base + 2u, base + 6u, base + 5u});
            faces.push_back({base + 0u, base + 4u, base + 7u, base + 3u});
        };

        auto append_horizontal_wall = [&](float x0, float z, float x1, float y_base)
        {
            append_box(x0, y_base, z - wall_thickness * 0.5f,
                       x1, y_base + WALL_HEIGHT, z + wall_thickness * 0.5f);
        };

        auto append_vertical_wall = [&](float x, float z0, float z1, float y_base)
        {
            append_box(x - wall_thickness * 0.5f, y_base, z0,
                       x + wall_thickness * 0.5f, y_base + WALL_HEIGHT, z1);
        };

        for (unsigned int lv = 0; lv < config.levels(); ++lv)
        {
            const float y_base = static_cast<float>(lv) * LEVEL_GAP;

            for (unsigned int row = 0; row < config.rows(); ++row)
            {
                for (unsigned int col = 0; col < config.columns(); ++col)
                {
                    const auto index = static_cast<int>(lv * config.rows() * config.columns() + row * config.columns() + col);
                    const auto current = grid_ops.search(index);
                    if (!current)
                    {
                        continue;
                    }

                    const float x0 = static_cast<float>(col);
                    const float x1 = static_cast<float>(col + 1u);
                    const float z0 = static_cast<float>(row);
                    const float z1 = static_cast<float>(row + 1u);

                    const auto north = grid_ops.get_north(current);
                    const auto south = grid_ops.get_south(current);
                    const auto east = grid_ops.get_east(current);
                    const auto west = grid_ops.get_west(current);

                    if (row == 0u && !(north && current->is_linked(north)))
                    {
                        append_horizontal_wall(x0, z0, x1, y_base);
                    }
                    if (col == 0u && !(west && current->is_linked(west)))
                    {
                        append_vertical_wall(x0, z0, z1, y_base);
                    }
                    if (!(east && current->is_linked(east)))
                    {
                        append_vertical_wall(x1, z0, z1, y_base);
                    }
                    if (!(south && current->is_linked(south)))
                    {
                        append_horizontal_wall(x0, z1, x1, y_base);
                    }
                }
            }
        }

        if (vertices.empty() || faces.empty())
        {
            return {};
        }

        std::ostringstream result;
        result << "# MazeBuilder Wavefront OBJ\n";
        result << "# rows=" << config.rows() << " columns=" << config.columns() << " levels=" << config.levels() << "\n\n";

        for (const auto &vertex : vertices)
        {
            result << "v " << vertex[0] << ' ' << vertex[1] << ' ' << vertex[2] << "\n";
        }

        result << "\n";
        for (const auto &face : faces)
        {
            result << "f " << face[0] << ' ' << face[1] << ' ' << face[2] << ' ' << face[3] << "\n";
        }

        m_result = result.str();
        grid_ops.set_file(m_result);
        return m_result;
    }
    catch (...)
    {
        return {};
    }
}

void wavefront_object_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool wavefront_object_create_state::update([[maybe_unused]] double delta_time) noexcept
{
    if (!processed_text_mapper)
    {
        request_stack_pop();
        return false;
    }

    const auto &parsed_args = state_utils::get_args_at_front(get_context());

    unsigned int rows = configurator::MAX_ROWS;
    unsigned int cols = configurator::MAX_COLUMNS;
    unsigned int levels = 1u;
    state_utils::parse_dimensions(std::cref(parsed_args), rows, cols, levels);

    const auto selected_grid_id = state_utils::has_distances(std::cref(parsed_args)) ? grid_identifier::DISTANCE : grid_identifier::BASIC;
    current_grid_id = selected_grid_id;

    std::string output_target = "stdout";
    if (!parsed_args.empty())
    {
        if (const auto it = parsed_args.find(mazes::args::OUTPUT_ID_WORD_STR); it != parsed_args.cend())
        {
            output_target = it->second;
        }
    }

    auto *rng_ptr = state_utils::get_rng_or_default(get_context());

    configurator cfg{};
    cfg.ensure_rows(rows)
        .ensure_columns(cols)
        .ensure_levels(levels)
        .ensure_distances(selected_grid_id == grid_identifier::DISTANCE);

    const std::string obj_result{create(cfg, *rng_ptr)};

    std::string final_result = obj_result;
    if (!obj_result.empty() && !output_target.empty() && output_target != "stdout")
    {
        if (io_utils::write_file(output_target, obj_result))
        {
            final_result = fmt::format("Wrote maze to {}", output_target);
        }
        else
        {
            final_result = fmt::format("Failed to write maze to {}", output_target);
        }
        global_async_logger().log_message(final_result);
    }

    if (processed_text_mapper)
    {
        try
        {
            auto &processing = processed_text_mapper->get(processed_text_identifier::FINISHED);
            processing.set_processed(final_result);
        }
        catch (...)
        {
        }
    }

    request_stack_pop();
    if (state_utils::advance_args(get_context()))
    {
        request_stack_push(state::ID::PARSING);
    }
    return false;
}
