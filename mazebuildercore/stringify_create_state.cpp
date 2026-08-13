#include <MazeBuilder/stringify_create_state.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder/state_utils.h>
#include <MazeBuilder/output_formats.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/string_utils.h>

#include <string>

using namespace mazes;

stringify_create_state::stringify_create_state(const runtime_app::context &ctx, runtime_stack *rs)
    : state(ctx, rs), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}
{
    state_utils::validate_mappers(grid_mapper, processed_text_mapper);
}

void stringify_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool stringify_create_state::update([[maybe_unused]] double delta_time) noexcept
{
    if (!processed_text_mapper)
    {
        request_stack_pop();
        return false;
    }

    const auto parsed_args = state_utils::get_args(get_context());

    std::string output_target;
    if (const auto parsed = parsed_args ? parsed_args->get() : std::nullopt; parsed.has_value())
    {
        if (const auto it = parsed->find(mazes::args::OUTPUT_ID_WORD_STR); it != parsed->cend())
        {
            output_target = it->second;
        }
    }

    unsigned int rows = configurator::MAX_ROWS;
    unsigned int cols = configurator::MAX_COLUMNS;
    unsigned int levels = 1u;
    state_utils::parse_dimensions(parsed_args, rows, cols, levels);

    auto *rng_ptr = state_utils::get_rng_or_default(get_context());

    configurator cfg{};
    cfg.ensure_rows(rows)
        .ensure_columns(cols)
        .ensure_levels(levels)
        .ensure_distances(state_utils::has_distances(parsed_args));

    const auto result = std::string{this->create(cfg, *rng_ptr)};

    if (!result.empty() && io_utils::is_an_absolute_path(output_target))
    {
        if (!io_utils::write_file(output_target, result))
        {
            global_async_logger().log_message("Failed to write maze to " + output_target);
        }
    }

    try
    {
        auto &processing_str = processed_text_mapper->get(processed_text_identifier::PROCESSING);
        if (!processing_str.is_processed())
        {
            processing_str.set_processed(result);
        }
    }
    catch (...)
    {
    }

    request_stack_pop();
    return false;
}

std::string_view stringify_create_state::create(const configurator &config, [[maybe_unused]] randomizer &rng) noexcept
{
    auto render_ascii_maze = [](const grid_interface &grid, const grid_operations &grid_ops,
                                const unsigned int rows, const unsigned int cols) -> std::string
    {
        auto pad_content = [](std::string content, const std::size_t cell_width) -> std::string
        {
            if (content.length() >= cell_width)
            {
                return content;
            }

            const auto remaining = cell_width - content.length();
            const auto left_pad = remaining / 2u;
            const auto right_pad = remaining - left_pad;

            content.insert(content.begin(), left_pad, ' ');
            content.append(right_pad, ' ');

            return content;
        };

        // Keep the classic compact maze by default, but widen when cell text requires it.
        std::size_t cell_width = 3u;
        for (unsigned int r = 0; r < rows; ++r)
        {
            for (unsigned int c = 0; c < cols; ++c)
            {
                if (const auto cp = grid_ops.search(static_cast<int>(r * cols + c)); cp)
                {
                    cell_width = std::max(cell_width, grid.contents_of(cp).length());
                }
            }
        }

        const std::string horizontal_wall(cell_width, '-');
        const std::string horizontal_gap(cell_width, ' ');

        std::string result;
        result.reserve((rows + 1u) * (cols * (cell_width + 1u) + 2u));

        result += "+";
        for (unsigned int c = 0; c < cols; ++c)
        {
            result += horizontal_wall;
            result += "+";
        }
        result += "\n";

        for (unsigned int r = 0; r < rows; ++r)
        {
            std::string top = "|", bottom = "+";
            for (unsigned int c = 0; c < cols; ++c)
            {
                const auto cp = grid_ops.search(static_cast<int>(r * cols + c));
                const auto e = cp ? grid_ops.get_east(cp) : nullptr;
                const auto s = cp ? grid_ops.get_south(cp) : nullptr;

                top += cp ? pad_content(grid.contents_of(cp), cell_width) : pad_content(" ", cell_width);
                top += (cp && e && cp->is_linked(e)) ? " " : "|";
                bottom += (cp && s && cp->is_linked(s)) ? horizontal_gap + "+" : horizontal_wall + "+";
            }

            result += top;
            result += '\n';
            result += bottom;
            result += '\n';
        }

        return result;
    };

    const auto grid_id = config.distances() ? grid_identifier::DISTANCE : grid_identifier::BASIC;
    try
    {
        auto &grid_ref = grid_mapper->get(grid_id);
        m_result = render_ascii_maze(std::cref(grid_ref), std::cref(grid_ref.operations()), config.rows(), config.columns());
        return m_result;
    }
    catch (...)
    {
        return {};
    }
}
