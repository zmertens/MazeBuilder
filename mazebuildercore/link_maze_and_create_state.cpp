#include <MazeBuilder/link_maze_and_create_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/mask.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/state_utils.h>
#include <MazeBuilder/string_utils.h>

using namespace mazes;

link_maze_and_create_state::link_maze_and_create_state(const runtime_app::context &ctx, runtime_stack *stack)
    : state(ctx, stack),
      args_mapper{ctx.get_args_manager()},
      grid_mapper{ctx.get_grid_manager()},
      processed_text_mapper{ctx.get_text_manager()},
      current_grid_id{grid_identifier::BASIC},
      m_use_distances{false},
      m_distances_start{configurator::DEFAULT_DISTANCES_START},
      m_distances_end{configurator::DEFAULT_DISTANCES_END}
{
    state_utils::validate_mappers(args_mapper, grid_mapper, processed_text_mapper);
}

link_maze_and_create_state::~link_maze_and_create_state() = default;

void link_maze_and_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool link_maze_and_create_state::update(randomizer &rng, [[maybe_unused]] double delta_time) noexcept
{
    const auto &parsed = args_mapper->get(args_identifier::PARSED);
    const auto parsed_args = parsed.front();

    const auto cfg = state_utils::build_configurator(parsed, get_algo_id());

    m_use_distances = cfg.distances();
    m_distances_start = cfg.distances_start();
    m_distances_end = cfg.distances_end();
    current_grid_id = m_use_distances ? grid_identifier::DISTANCE : grid_identifier::BASIC;

    state_utils::check_before_resize(&grid_mapper->get(current_grid_id).operations(), cfg.rows(), cfg.columns(), cfg.levels());

    auto *rng_ptr = &rng;
    state_utils::reseed_from_args(parsed_args, *rng_ptr);

    request_stack_pop();

    if (const auto result = create_as<std::string>(parsed, *rng_ptr); result && !result->empty())
    {
        processed_text_mapper->get(processed_text_identifier::PROCESSING).set(*result);
    }

    return true;
}
