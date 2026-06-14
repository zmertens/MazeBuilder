#include <MazeBuilder/loading_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/runtime_stack.h>

#include <array>

using namespace mazes;

loading_state::loading_state(const runtime_app::context &ctx, runtime_stack *rs)
    : state(ctx, rs), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}
{
}

void loading_state::draw() const noexcept
{
    // No visual output for now
}

bool loading_state::update(const std::optional<args> &args, [[maybe_unused]] double delta_time) noexcept
{
    if (!args.has_value())
    {
        request_stack_pop();
        return true;
    }

    if (!has_finished)
    {
        std::call_once(resource_loaded_flag, [this, &args, &delta_time]()
                       { load_resources(std::cref(args)); });

        has_finished = true;
        // Pop loading first, then push parsing so apply_pending_changes()
        // removes loading before adding parsing.
        request_stack_pop();
        request_stack_push(state::ID::PARSING);
    }

    return true;
}

void loading_state::load_resources(const std::optional<args> & /*args*/) noexcept
{
    // Set the grid mapper first
    if (grid_mapper)
    {
        // load() calls grid_interface::create(), which pre-generates the grid and stores a pointer in the mapper.
        try
        {
            grid_mapper->load<grid>(grid_identifier::BASIC, configurator::MAX_ROWS, configurator::MAX_COLUMNS, configurator::MAX_LEVELS);
            grid_mapper->load<colored_grid>(grid_identifier::COLORED, configurator::MAX_ROWS, configurator::MAX_COLUMNS, configurator::MAX_LEVELS);
            grid_mapper->load<distance_grid>(grid_identifier::DISTANCE, configurator::MAX_ROWS, configurator::MAX_COLUMNS, configurator::MAX_LEVELS);
        }
        catch (...)
        {
        }

        // Grids are loaded at max size. PARSING is pushed from update() after pop.
    }

    // Pre-allocate processed_text slots so any state can safely call
    // processed_text_mapper->get(id) without hitting the assert.
    if (processed_text_mapper)
    {
        static constexpr std::array all_text_ids = {
            processed_text_identifier::FINISHED,
            processed_text_identifier::GARBAGE,
            processed_text_identifier::PROCESSING,
        };

        for (auto id : all_text_ids)
        {
            try
            {
                processed_text_mapper->load(id, std::string_view{});
            }
            catch (...)
            {
            }
        }
    }
}