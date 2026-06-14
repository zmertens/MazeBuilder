#include <MazeBuilder/objectify_create_state.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_stack.h>

using namespace mazes;

objectify_create_state::objectify_create_state(const runtime_app::context &ctx, runtime_stack *stack)
: state(ctx, stack), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}
{

}

void objectify_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool objectify_create_state::update([[maybe_unused]] const std::optional<args> &args, [[maybe_unused]] double delta_time) noexcept
{
    // Implementation of the update function
    return true;
}

std::string_view objectify_create_state::create([[maybe_unused]] algo a, unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept
{
    auto sum = rows + cols + levels;
    [[maybe_unused]] auto sum1 = rng(0, sum);
    return std::string_view{"Objectify maze creation not yet implemented"};
}
