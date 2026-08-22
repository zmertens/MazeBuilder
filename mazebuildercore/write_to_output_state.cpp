#include <MazeBuilder/write_to_output_state.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/state_utils.h>

using namespace mazes;

write_to_output_state::write_to_output_state(const runtime_app::context& ctx, runtime_stack* stack)
    : state(ctx, stack),
    grid_mapper{ ctx.get_grid_manager() },
    processed_text_mapper{ ctx.get_text_manager() },
    current_grid_id{ grid_identifier::BASIC }
{
    state_utils::validate_mappers(grid_mapper, processed_text_mapper);
}

void write_to_output_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool write_to_output_state::update([[maybe_unused]] double delta_time) noexcept
{
    m_result.clear();

    if (!grid_mapper || !processed_text_mapper)
    {
        request_stack_pop();
        return false;
    }

    const auto& parsed_args = state_utils::get_args_at_front(get_context());

    unsigned int rows = configurator::MAX_ROWS;
    unsigned int cols = configurator::MAX_COLUMNS;
    unsigned int levels = 1u;

    state_utils::parse_dimensions(std::cref(parsed_args), rows, cols, levels);

    current_grid_id = state_utils::has_distances(std::cref(parsed_args)) ? grid_identifier::DISTANCE : grid_identifier::BASIC;

    try
    {
        const auto [grid_rows, grid_cols, grid_levels] = grid_mapper->get(current_grid_id).operations().get_dimensions();
        rows = grid_rows;
        cols = grid_cols;
        levels = grid_levels;
    }
    catch (...)
    {
    }

    std::string output_target;
    if (const auto it = parsed_args.find(mazes::args::OUTPUT_ID_WORD_STR); it != parsed_args.cend())
    {
        output_target = it->second;
    }

    auto* rng_ptr = state_utils::get_rng_or_default(get_context());

    configurator cfg{};
    cfg.ensure_rows(rows)
        .ensure_columns(cols)
        .ensure_levels(levels)
        .ensure_distances(current_grid_id == grid_identifier::DISTANCE);

    m_result = std::string{ create(cfg, *rng_ptr) };

    // Attempt to write output if result is not empty
    if (!m_result.empty())
    {
        if (bool write_success = write_output(output_target, m_result); !write_success)
        {
            global_async_logger().log_message("Failed to write maze to " + output_target);
        }

        if (!output_target.empty() && output_format_or_default(output_target) != output_format::STDOUT)
        {
            global_async_logger().log(m_result);
        }
    }

    processed_text_mapper->get(processed_text_identifier::FINISHED).set(m_result);

    request_stack_pop();
    if (state_utils::advance_args(get_context()))
    {
        request_stack_push(state::ID::PARSE);
    }

    return true;
}
