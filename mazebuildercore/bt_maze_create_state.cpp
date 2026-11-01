#include <MazeBuilder/bt_maze_create_state.h>

#include <MazeBuilder/lab.h>

#include <string>

using namespace mazes;

bt_maze_create_state::bt_maze_create_state(const runtime_app::context &ctx, runtime_stack *rs)
    : link_maze_and_create_state(ctx, rs)
{
}

algo bt_maze_create_state::get_algo_id() const noexcept
{
    return algo::BINARY_TREE;
}

std::any bt_maze_create_state::create([[maybe_unused]] const args &a, randomizer &rng) noexcept
{
    return run_link_algorithm([&rng](grid_operations &ops)
                              { return lab::binary_tree(ops, rng); });
}
