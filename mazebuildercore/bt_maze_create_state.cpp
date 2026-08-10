#include <MazeBuilder/bt_maze_create_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/state_utils.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_stack.h>

#include <string>
#include <vector>

using namespace mazes;

bt_maze_create_state::bt_maze_create_state(const runtime_app::context &ctx, runtime_stack *rs)
    : state(ctx, rs),
      grid_mapper{ctx.get_grid_manager()},
      processed_text_mapper{ctx.get_text_manager()},
      m_grid_id{grid_identifier::BASIC},
      m_use_distances{false},
      m_distances_start{configurator::DEFAULT_DISTANCES_START},
      m_distances_end{configurator::DEFAULT_DISTANCES_END}
{
}

std::string_view bt_maze_create_state::create(const configurator &config, randomizer &rng) noexcept
{
    return create_bt_maze(config.rows(), config.columns(), config.levels(), rng);
}

void bt_maze_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool bt_maze_create_state::update(const std::optional<args> &args,
                                  [[maybe_unused]] double delta_time) noexcept
{
    if (!args.has_value() || !grid_mapper || !processed_text_mapper)
    {
        request_stack_pop();
        return false;
    }

    unsigned int rows = configurator::MAX_ROWS;
    unsigned int cols = configurator::MAX_COLUMNS;
    unsigned int levels = 1u;

    state_utils::parse_dimensions(args, rows, cols, levels);
    m_use_distances = state_utils::has_distances(args);
    m_grid_id = m_use_distances ? grid_identifier::DISTANCE : grid_identifier::BASIC;

    const auto distance_settings = state_utils::parse_distance_settings(args);
    m_distances_start = distance_settings.start;
    m_distances_end = distance_settings.end;

    try
    {
        grid_mapper->get(m_grid_id).operations().resize(rows, cols, levels);
    }
    catch (...)
    {
        request_stack_pop();
        return false;
    }

    randomizer fallback_rng{};
    auto *rng_ptr = state_utils::get_rng_or_default(get_context(), fallback_rng);

    configurator cfg{};
    cfg.ensure_rows(rows)
        .ensure_columns(cols)
        .ensure_levels(levels)
        .ensure_distances(m_use_distances)
        .ensure_distances_start(m_distances_start)
        .ensure_distances_end(m_distances_end)
        .ensure_algo_id(algo::BINARY_TREE);

    auto result = create(cfg, *rng_ptr);
    if (!result.empty())
    {
        try
        {
            request_stack_pop();
            request_stack_push(state_utils::output_state_for(args));
        }
        catch (...)
        {
        }
    }
    else
    {
        request_stack_pop();
    }
    return false;
}

std::string_view bt_maze_create_state::create_bt_maze(unsigned int rows, unsigned int cols, unsigned int levels,
                                                      randomizer &rng) noexcept
{
    if (!grid_mapper)
        return {};
    try
    {
        auto &grid_ref = grid_mapper->get(m_grid_id);
        auto &grid_ops = grid_ref.operations();

        for (unsigned int lv = 0; lv < levels; ++lv)
        {
            for (unsigned int row = 0; row < rows; ++row)
            {
                for (unsigned int col = 0; col < cols; ++col)
                {
                    auto c = grid_ops.search(static_cast<int>(lv * rows * cols + row * cols + col));
                    if (!c)
                        continue;
                    std::vector<std::shared_ptr<cell>> cands;
                    if (auto n = grid_ops.get_north(c))
                        cands.push_back(n);
                    if (auto e = grid_ops.get_east(c))
                        cands.push_back(e);
                    if (!cands.empty())
                        lab::link(c, cands.at(static_cast<size_t>(rng(0, static_cast<int>(cands.size()) - 1))), true);
                }
            }
        }

        if (m_use_distances)
        {
            if (auto *distance_grid_ref = dynamic_cast<distance_grid *>(&grid_ref))
            {
                distance_grid_ref->calculate_distances(m_distances_start, m_distances_end);
            }
        }

        return std::string_view{"Maze generated"};
    }
    catch (...)
    {
        return {};
    }
}
