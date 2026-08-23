#include <MazeBuilder/state.h>

#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/runtime_stack.h>

void mazes::state::request_stack_push(const mazes::state::ID state_id) const noexcept
{
    _runtime_stack->push_state(state_id);
}

void mazes::state::request_stack_pop() const noexcept
{
    _runtime_stack->pop_state();
}

void mazes::state::request_stack_clear() const noexcept
{
    _runtime_stack->clear_states();
}

const mazes::runtime_app::context& mazes::state::get_context() const noexcept { return ctx; }

mazes::runtime_stack& mazes::state::get_stack() const noexcept { return *_runtime_stack; }
