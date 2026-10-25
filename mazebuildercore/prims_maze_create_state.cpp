#include <MazeBuilder/prims_maze_create_state.h>

#include <MazeBuilder/state_utils.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>

#include <span>
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

std::any prims_maze_create_state::create(const args &a, randomizer& rng) noexcept
{
    const auto config = state_utils::build_configurator(a, get_algo_id());
    return std::string{create_prims_maze(config.rows(), config.columns(), config.levels(), rng)};
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

        const int level_size = static_cast<int>(rows * cols);
        if (level_size <= 0)
        {
            return {};
        }

        // Each level is an independent maze
        for (unsigned int lv = 0; lv < levels; ++lv)
        {
            const int offset = static_cast<int>(lv) * level_size;
            const auto start = grid_ops.search(offset + rng(0, level_size - 1));
            if (!start)
            {
                return {};
            }

            std::unordered_set<int> in_maze;
            std::unordered_set<int> frontier_ids;
            std::vector<std::shared_ptr<cell>> frontier;

            grid_operations::neighbor_buffer neighbor_buf;
            auto add_frontier_neighbors = [&](const std::shared_ptr<cell>& c)
                {
                    const auto count = grid_ops.collect_neighbors(c, neighbor_buf);
                    for (const auto& n : std::span(neighbor_buf).first(count))
                    {
                        if (!n)
                        {
                            continue;
                        }

                        const auto idx = grid_ops.index_of(n);
                        if (!in_maze.contains(idx) && frontier_ids.insert(idx).second)
                        {
                            frontier.push_back(n);
                        }
                    }
                };

            in_maze.insert(grid_ops.index_of(start));
            add_frontier_neighbors(start);

            while (!frontier.empty())
            {
                const auto frontier_pos = static_cast<std::size_t>(rng(0, static_cast<int>(frontier.size()) - 1));
                auto current = frontier.at(frontier_pos);

                frontier.at(frontier_pos) = frontier.back();
                frontier.pop_back();
                frontier_ids.erase(grid_ops.index_of(current));

                std::vector<std::shared_ptr<cell>> neighbors_in_maze;
                const auto current_count = grid_ops.collect_neighbors(current, neighbor_buf);
                for (const auto& n : std::span(neighbor_buf).first(current_count))
                {
                    if (n && in_maze.contains(grid_ops.index_of(n)))
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
                in_maze.insert(grid_ops.index_of(current));

                add_frontier_neighbors(current);
            }
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
