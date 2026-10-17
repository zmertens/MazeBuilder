#include <MazeBuilder/dfs_maze_create_state.h>

#include <MazeBuilder/state_utils.h>
#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>

#include <stack>
#include <span>
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

std::any dfs_maze_create_state::create(const args &a, randomizer& rng) noexcept
{
    const auto config = state_utils::build_configurator(a, get_algo_id());
    return std::string{create_dfs_maze(config.rows(), config.columns(), config.levels(), rng)};
}

std::string_view dfs_maze_create_state::create_dfs_maze(const unsigned int rows, const unsigned int cols,
    const unsigned int levels, randomizer& rng) noexcept
{
    if (!grid_mapper)
        return {};
    try
    {
        auto& grid_ref = grid_mapper->get(current_grid_id);
        auto& grid_ops = grid_ref.operations();

        // Each level is an independent maze
        const int level_size = static_cast<int>(rows * cols);
        for (unsigned int lv = 0; lv < levels; ++lv)
        {
            const int offset = static_cast<int>(lv) * level_size;
            const auto start = grid_ops.search(offset + rng(0, level_size - 1));
            if (!start)
            {
                return {};
            }

            std::stack<std::shared_ptr<cell>> stk;
            std::unordered_set<int> visited;
            stk.push(start);
            visited.insert(start->get_index());

            grid_operations::neighbor_buffer neighbor_buf;
            while (!stk.empty())
            {
                auto current = stk.top();
                const auto neighbor_count = grid_ops.collect_neighbors(current, neighbor_buf);

                std::vector<std::shared_ptr<cell>> unvisited;
                for (const auto& n : std::span(neighbor_buf).first(neighbor_count))
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
