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

link_maze_and_create_state::link_maze_and_create_state(const runtime_app::context& ctx, runtime_stack* stack)
    : state(ctx, stack),
    grid_mapper{ ctx.get_grid_manager() },
    processed_text_mapper{ ctx.get_text_manager() },
    current_grid_id{ grid_identifier::BASIC },
    m_use_distances{ false },
    m_distances_start{ configurator::DEFAULT_DISTANCES_START },
    m_distances_end{ configurator::DEFAULT_DISTANCES_END }
{
    state_utils::validate_mappers(grid_mapper, processed_text_mapper);
}

void link_maze_and_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool link_maze_and_create_state::update([[maybe_unused]] double delta_time) noexcept
{
    if (!grid_mapper || !processed_text_mapper)
    {
        request_stack_pop();
        return false;
    }

    const auto& parsed_args = state_utils::get_args_at_front(get_context());

    unsigned int rows = configurator::MAX_ROWS;
    unsigned int cols = configurator::MAX_COLUMNS;
    unsigned int levels = 1u;

    std::string mask_file;
    if (const auto mask_it = parsed_args.find(mazes::args::MASK_WORD_STR); mask_it != parsed_args.cend())
    {
        mask_file = mask_it->second;
        if (!mask_file.empty())
        {
            try
            {
                auto temp_mask = mask::from_txt(mask_file);
                rows = temp_mask.rows();
                cols = temp_mask.columns();
            }
            catch (...)
            {
            }
        }
    }

    if (mask_file.empty())
    {
        state_utils::parse_dimensions(std::cref(parsed_args), std::ref(rows), std::ref(cols), std::ref(levels));
    }

    m_use_distances = state_utils::has_distances(std::cref(parsed_args));
    current_grid_id = m_use_distances ? grid_identifier::DISTANCE : grid_identifier::BASIC;

    const auto distance_settings = state_utils::parse_distance_settings(std::cref(parsed_args));
    m_distances_start = distance_settings.start;
    m_distances_end = distance_settings.end;

    state_utils::check_before_resize(&grid_mapper->get(current_grid_id).operations(), rows, cols, levels);

    auto* rng_ptr = state_utils::get_rng_or_default(get_context());
    state_utils::reseed_from_args(parsed_args, *rng_ptr);

    auto selected_algo = get_algo_id();
    if (const auto algo_it = parsed_args.find(mazes::args::ALGO_ID_WORD_STR); algo_it != parsed_args.cend())
    {
        try
        {
            selected_algo = to_algo_from_sv(algo_it->second);
        }
        catch (...)
        {
        }
    }

    configurator cfg = configurator{}
        .ensure_rows(rows)
        .ensure_columns(cols)
        .ensure_levels(levels)
        .ensure_distances(m_use_distances)
        .ensure_distances_start(m_distances_start)
        .ensure_distances_end(m_distances_end)
        .ensure_algo_id(selected_algo)
        .ensure_mask_file(mask_file);

    request_stack_pop();

    if (const auto result{ create(cfg, *rng_ptr) }; !result.empty())
    {
        m_result = std::string{ result };
        processed_text_mapper->get(processed_text_identifier::FINISHED).set(m_result);

        if (!std::string_view{ m_result }.starts_with("Error"))
        {
            request_stack_push(state_utils::output_state_for(parsed_args));
        }
    }

    return true;
}
