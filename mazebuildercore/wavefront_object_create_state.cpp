#include <MazeBuilder/wavefront_object_create_state.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder/state_utils.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_stack.h>

#include <sstream>
#include <tuple>
#include <vector>

using namespace mazes;

wavefront_object_create_state::wavefront_object_create_state(const runtime_app::context &ctx, runtime_stack *rs)
    : state(ctx, rs), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}
{
}

std::string_view wavefront_object_create_state::create(const configurator &config, randomizer &rng) noexcept
{
    if (!grid_mapper)
    {
        return {};
    }

    try
    {
        auto &grid_ref = grid_mapper->get(m_grid_id);
        auto &grid_ops = grid_ref.operations();

        constexpr float wall_height = 1.0f;
        constexpr float level_gap = 2.0f;
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
                       x1, y_base + wall_height, z + wall_thickness * 0.5f);
        };

        auto append_vertical_wall = [&](float x, float z0, float z1, float y_base)
        {
            append_box(x - wall_thickness * 0.5f, y_base, z0,
                       x + wall_thickness * 0.5f, y_base + wall_height, z1);
        };

        for (unsigned int lv = 0; lv < config.levels(); ++lv)
        {
            const float y_base = static_cast<float>(lv) * level_gap;

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

bool wavefront_object_create_state::update(const std::optional<args> &args, [[maybe_unused]] double delta_time) noexcept
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
    state_utils::parse_dimensions(args, rows, cols, levels);

    m_grid_id = state_utils::has_distances(args) ? grid_identifier::DISTANCE : grid_identifier::BASIC;

    std::string output_target;
    if (const auto parsed = args->get(); parsed.has_value())
    {
        if (const auto it = parsed->find(mazes::args::OUTPUT_ID_WORD_STR); it != parsed->cend())
        {
            output_target = it->second;
        }
    }

    randomizer fallback_rng{};
    auto *rng_ptr = state_utils::get_rng_or_default(get_context(), fallback_rng);

    configurator cfg{};
    cfg.ensure_rows(rows)
        .ensure_columns(cols)
        .ensure_levels(levels)
        .ensure_distances(m_grid_id == grid_identifier::DISTANCE);

    m_result = std::string{create(std::cref(cfg), *rng_ptr)};

    if (!m_result.empty() && !output_target.empty())
    {
        const auto normalized_output = io_utils::normalize_path(output_target);
        if (io_utils::write_file(normalized_output, m_result))
        {
            m_result = "Wrote maze to " + normalized_output;
        }
        else
        {
            m_result = "Failed to write maze to " + normalized_output;
        }
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
