#include <MazeBuilder/masked_maze_and_create_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/mask.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/state_utils.h>

#include <string>

using namespace mazes;

masked_maze_and_create_state::masked_maze_and_create_state(const runtime_app::context &ctx, runtime_stack *rs)
    : link_maze_and_create_state(ctx, rs)
{
}

algo masked_maze_and_create_state::get_algo_id() const noexcept
{
    return algo::BINARY_TREE;
}

std::any masked_maze_and_create_state::create(const args &a, randomizer &rng) noexcept
{
    const auto config = state_utils::build_configurator(a, get_algo_id());
    const auto mask_source = config.mask_source();
    if (mask_source.empty())
    {
        return std::string{"Error: No mask source specified"};
    }

    try
    {
        const auto loaded_mask = mask::from_source(mask_source);

        // The mask applies to every level of the grid
        const auto level_columns = static_cast<int>(loaded_mask.columns());
        const auto level_size = static_cast<int>(loaded_mask.rows()) * level_columns;
        const auto open = [&loaded_mask, level_columns, level_size](const int index)
        {
            const int in_level = index % level_size;
            return loaded_mask(static_cast<unsigned int>(in_level / level_columns),
                               static_cast<unsigned int>(in_level % level_columns));
        };

        const auto algorithm = config.algo_id();
        auto result = run_link_algorithm([&](grid_operations &ops)
                                         {
            switch (algorithm)
            {
            case algo::BINARY_TREE:
                return lab::binary_tree(ops, rng, open);
            case algo::SIDEWINDER:
                return lab::sidewinder(ops, rng, open);
            case algo::DFS:
                return lab::dfs(ops, rng, open);
            case algo::PRIMS:
                return lab::prims(ops, rng, open);
            default:
                return false;
            } });

        return result.empty() ? std::string{"Error: Could not create masked maze"} : result;
    }
    catch (const std::exception &)
    {
        return std::string{"Error: Could not load mask (file not found relative to the working directory, or empty)"};
    }
}
