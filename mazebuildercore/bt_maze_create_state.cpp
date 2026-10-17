#include <MazeBuilder/bt_maze_create_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_stack.h>

#include <array>
#include <string>
#include <vector>

using namespace mazes;

bt_maze_create_state::bt_maze_create_state(const runtime_app::context& ctx, runtime_stack* rs)
    : link_maze_and_create_state(ctx, rs)
{
}

algo bt_maze_create_state::get_algo_id() const noexcept
{
    return algo::BINARY_TREE;
}

std::string_view bt_maze_create_state::create(const configurator& config, randomizer& rng) noexcept
{
    return create_bt_maze(config.rows(), config.columns(), config.levels(), rng);
}

std::string_view bt_maze_create_state::create_bt_maze([[maybe_unused]] unsigned int rows, [[maybe_unused]] unsigned int cols, [[maybe_unused]] unsigned int levels,
    randomizer& rng) noexcept
{
    if (!grid_mapper)
        return {};
    try
    {
        auto& grid_ref = grid_mapper->get(current_grid_id);
        auto& grid_ops = grid_ref.operations();

        std::array<std::shared_ptr<cell>, 2> cands;
        for (const auto& c : grid_ops.cells())
        {
            if (!c)
                continue;
            std::size_t n = 0;
            if (auto north = grid_ops.get_north(c))
                cands[n++] = std::move(north);
            if (auto east = grid_ops.get_east(c))
                cands[n++] = std::move(east);
            if (n > 0)
                lab::link(c, cands[static_cast<size_t>(rng(0, static_cast<int>(n) - 1))], true);
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
