#include <MazeBuilder/dfs_maze_create_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/maze_state_utils.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_stack.h>

#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

using namespace mazes;

dfs_maze_create_state::dfs_maze_create_state(const runtime_app::context& ctx, runtime_stack* rs)
    : state(ctx, rs), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}
{
}

std::string_view dfs_maze_create_state::create(const configurator& config, randomizer& rng) noexcept
{
    return create_dfs_maze(config.rows(), config.columns(), config.levels(), rng);
}

void dfs_maze_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool dfs_maze_create_state::update([[maybe_unused]] const std::optional<args>& args,
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

    maze_state_utils::parse_dimensions(args, rows, cols, levels);

    m_use_distances = maze_state_utils::has_distances(args);
    m_grid_id = m_use_distances ? grid_identifier::DISTANCE : grid_identifier::BASIC;
    const auto distance_settings = maze_state_utils::parse_distance_settings(args);
    m_distances_start = distance_settings.start;
    m_distances_end = distance_settings.end;

    try { grid_mapper->get(m_grid_id).operations().resize(rows, cols, levels); }
    catch (...)
    {
        request_stack_pop();
        return false;
    }

    randomizer fallback_rng{};
    auto* rng_ptr = maze_state_utils::get_rng_or_default(get_context(), fallback_rng);

    configurator cfg{};
    cfg.ensure_rows(rows)
        .ensure_columns(cols)
        .ensure_levels(levels)
        .ensure_distances(m_use_distances)
        .ensure_distances_start(m_distances_start)
        .ensure_distances_end(m_distances_end)
        .ensure_algo_id(algo::DFS);

    if (const auto result = create(cfg, *rng_ptr); !result.empty())
    {
        try
        {
            request_stack_pop();
            request_stack_push(maze_state_utils::output_state_for(args));
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

std::string_view dfs_maze_create_state::create_dfs_maze(unsigned int rows, unsigned int cols, unsigned int levels,
                                                        randomizer& rng) noexcept
{
    [[maybe_unused]] auto _levels = levels; // DFS operates on level 0; multi-level support is future work
    if (!grid_mapper) return {};
    try
    {
        auto& grid_ref = grid_mapper->get(m_grid_id);
        auto& grid_ops = grid_ref.operations();

        // Start in the first cell of level 0
        const int level0_max = static_cast<int>(rows * cols) - 1;
        const auto start = grid_ops.search(rng(0, level0_max));
        if (!start)
        {
            return {};
        }

        std::stack<std::shared_ptr<cell>> stk;
        std::unordered_set<int> visited;
        stk.push(start);
        visited.insert(start->get_index());

        while (!stk.empty())
        {
            auto current = stk.top();
            auto all_neighbors = grid_ops.get_neighbors(current);

            std::vector<std::shared_ptr<cell>> unvisited;
            for (auto& n : all_neighbors)
            {
                if (n && !visited.contains(n->get_index()))
                {
                    unvisited.push_back(n);
                }
            }

            if (unvisited.empty())
            {
                stk.pop();
            }
            else
            {
                auto& chosen = unvisited.at(static_cast<size_t>(rng(0, static_cast<int>(unvisited.size()) - 1)));
                lab::link(current, chosen, true);
                visited.insert(chosen->get_index());
                stk.push(chosen);
            }
        }

        if (m_use_distances && m_grid_id == grid_identifier::DISTANCE)
        {
            if (auto* distance_grid_ref = dynamic_cast<distance_grid*>(&grid_ref))
            {
                distance_grid_ref->calculate_distances(m_distances_start, m_distances_end);
            }
        }

        return std::string_view{"Maze generated"};
    }
    catch (...) { return {}; }
}
