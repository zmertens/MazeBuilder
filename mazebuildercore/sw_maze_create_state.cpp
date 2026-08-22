#include <MazeBuilder/sw_maze_create_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>

#include <string>
#include <vector>

using namespace mazes;

sw_maze_create_state::sw_maze_create_state(const runtime_app::context& ctx, runtime_stack* rs)
    : link_maze_and_create_state(ctx, rs)
{
}

algo sw_maze_create_state::get_algo_id() const noexcept
{
    return algo::SIDEWINDER;
}

std::string_view sw_maze_create_state::create(const configurator& config, randomizer& rng) noexcept
{
    return create_sw_maze(config.rows(), config.columns(), config.levels(), rng);
}

std::string_view sw_maze_create_state::create_sw_maze(const unsigned int rows, const unsigned int cols,
    const unsigned int levels,
    randomizer& rng) noexcept
{
    if (!grid_mapper)
        return {};
    try
    {
        auto& grid_ref = grid_mapper->get(current_grid_id);
        auto& grid_ops = grid_ref.operations();

        for (unsigned int lv = 0; lv < levels; ++lv)
        {
            for (unsigned int row = 0; row < rows; ++row)
            {
                std::vector<std::shared_ptr<cell>> run;

                for (unsigned int col = 0; col < cols; ++col)
                {
                    auto c = grid_ops.search(static_cast<int>(lv * rows * cols + row * cols + col));
                    if (!c)
                        continue;
                    run.push_back(c);

                    const bool at_east = (col == cols - 1);
                    const bool at_north = (row == 0);

                    if (at_east || (!at_north && rng(0, 1) == 0))
                    {
                        if (!at_north && !run.empty())
                        {
                            auto& m = run.at(static_cast<size_t>(rng(0, static_cast<int>(run.size()) - 1)));
                            if (auto n = grid_ops.get_north(m))
                                lab::link(m, n, true);
                        }
                        run.clear();
                    } else
                    {
                        if (auto e = grid_ops.get_east(c))
                            lab::link(c, e, true);
                    }
                }
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
    } catch (...)
    {
        return {};
    }
}
