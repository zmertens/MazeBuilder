#include <MazeBuilder/dfs_maze_create_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>

#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

using namespace mazes;

dfs_maze_create_state::dfs_maze_create_state(const runtime_app::context& ctx, runtime_stack* rs)
    : link_maze_and_create_state(ctx, rs)
{
}

algo dfs_maze_create_state::get_algo_id() const noexcept
{
    return algo::DFS;
}

std::string_view dfs_maze_create_state::create(const configurator& config, randomizer& rng) noexcept
{
    return create_dfs_maze(config.rows(), config.columns(), config.levels(), rng);
}

std::string_view dfs_maze_create_state::create_dfs_maze(unsigned int rows, unsigned int cols, unsigned int levels,
    randomizer& rng) noexcept
{
    [[maybe_unused]] auto _levels = levels; // DFS operates on level 0; multi-level support is future work
    if (!grid_mapper)
        return {};
    try
    {
        auto& grid_ref = grid_mapper->get(current_grid_id);
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
            } else
            {
                auto& chosen = unvisited.at(static_cast<size_t>(rng(0, static_cast<int>(unvisited.size()) - 1)));
                lab::link(current, chosen, true);
                visited.insert(chosen->get_index());
                stk.push(chosen);
            }
        }

        if (m_use_distances && current_grid_id == grid_identifier::DISTANCE)
        {
            if (auto* distance_grid_ref = dynamic_cast<distance_grid*>(&grid_ref))
            {
                distance_grid_ref->calculate_distances(m_distances_start, m_distances_end);
            }
        }

        return std::string_view{ "Maze generated" };
    } catch (...)
    {
        return {};
    }
}
