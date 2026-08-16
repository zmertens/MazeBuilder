#include <MazeBuilder/loading_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/state_utils.h>

#include <array>

#include <fmt/format.h>

using namespace mazes;

loading_state::loading_state(const runtime_app::context &ctx, runtime_stack *rs)
    : state(ctx, rs), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}, args_mapper{ctx.get_args_manager()}
{
    state_utils::validate_mappers(args_mapper, grid_mapper, processed_text_mapper);
}

void loading_state::draw() const noexcept
{
    // No visual output for now
}

bool loading_state::update([[maybe_unused]] double delta_time) noexcept
{
    if (!has_finished)
    {
        std::call_once(resource_loaded_flag, [this]()
                       { load_resources(); });

        has_finished = true;
        request_stack_pop();
    }

    return false;
}

void loading_state::load_resources() const noexcept
{
    args_mapper->load<args>(args_identifier::RAW);
    args_mapper->load<args>(args_identifier::PARSED);

    grid_mapper->load<grid>(grid_identifier::BASIC, configurator::MAX_ROWS, configurator::MAX_COLUMNS,
                            configurator::MAX_LEVELS);
    grid_mapper->load<colored_grid>(grid_identifier::COLORED, configurator::MAX_ROWS, configurator::MAX_COLUMNS,
                                    configurator::MAX_LEVELS);
    grid_mapper->load<distance_grid>(grid_identifier::DISTANCE, configurator::MAX_ROWS,
                                     configurator::MAX_COLUMNS, configurator::MAX_LEVELS);

    processed_text_mapper->load<processed_text>(processed_text_identifier::FINISHED);
    processed_text_mapper->load<processed_text>(processed_text_identifier::PROCESSING);
    processed_text_mapper->load<processed_text>(processed_text_identifier::UNKNOWN);
}

void loading_state::set_text_to_unknown(const std::string_view txt) noexcept
{
    if (processed_text_mapper)
    {
        try
        {
            auto &unknown_text = processed_text_mapper->get(processed_text_identifier::UNKNOWN);
            unknown_text.set(txt);
        }
        catch (...)
        {
            async_logger().log("Failed to set text to UNKNOWN in loading_state.");
        }
    }
}