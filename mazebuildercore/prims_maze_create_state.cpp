#include <MazeBuilder/prims_maze_create_state.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>

#include <string>
#include <unordered_set>
#include <vector>

using namespace mazes;

prims_maze_create_state::prims_maze_create_state(const runtime_app::context& ctx, runtime_stack* rs)
    : link_maze_and_create_state(ctx, rs)
{
}

algo prims_maze_create_state::get_algo_id() const noexcept
{
    return algo::PRIMS;
}

std::string_view prims_maze_create_state::create(const configurator& config, randomizer& rng) noexcept
{
    return create_prims_maze(config.rows(), config.columns(), config.levels(), rng);
}

std::string_view prims_maze_create_state::create_prims_maze(const unsigned int rows, const unsigned int cols,
    const unsigned int levels,
    randomizer& rng) noexcept
{
    if (!grid_mapper)
        return {};

    try
    {
        auto& grid_ref = grid_mapper->get(current_grid_id);
        auto& grid_ops = grid_ref.operations();

        const int total_cells = static_cast<int>(rows * cols * levels);
        if (total_cells <= 0)
        {
            return {};
        }

        const auto start = grid_ops.search(rng(0, total_cells - 1));
        if (!start)
        {
            return {};
        }

        std::unordered_set<int> in_maze;
        std::unordered_set<int> frontier_ids;
        std::vector<std::shared_ptr<cell>> frontier;

        auto add_frontier_neighbors = [&](const std::shared_ptr<cell>& c)
            {
                for (const auto& n : grid_ops.get_neighbors(c))
                {
                    if (!n)
                    {
                        continue;
                    }

                    const auto idx = n->get_index();
                    if (!in_maze.contains(idx) && frontier_ids.insert(idx).second)
                    {
                        frontier.push_back(n);
                    }
                }
            };

        in_maze.insert(start->get_index());
        add_frontier_neighbors(start);

        while (!frontier.empty())
        {
            const auto frontier_pos = static_cast<std::size_t>(rng(0, static_cast<int>(frontier.size()) - 1));
            auto current = frontier.at(frontier_pos);

            frontier.at(frontier_pos) = frontier.back();
            frontier.pop_back();
            frontier_ids.erase(current->get_index());

            std::vector<std::shared_ptr<cell>> neighbors_in_maze;
            for (const auto& n : grid_ops.get_neighbors(current))
            {
                if (n && in_maze.contains(n->get_index()))
                {
                    neighbors_in_maze.push_back(n);
                }
            }

            if (neighbors_in_maze.empty())
            {
                continue;
            }

            const auto chosen_idx = static_cast<std::size_t>(rng(0, static_cast<int>(neighbors_in_maze.size()) - 1));
            lab::link(current, neighbors_in_maze.at(chosen_idx), true);
            in_maze.insert(current->get_index());

            add_frontier_neighbors(current);
        }

        if (m_use_distances)
        {
            if (auto* distance_grid_ref = dynamic_cast<distance_grid*>(&grid_ref))
            {
                distance_grid_ref->calculate_distances(m_distances_start, m_distances_end);
            }
        }

        return std::string_view{ "Maze generated" };
    }
    catch (...)
    {
        return {};
    }
}
