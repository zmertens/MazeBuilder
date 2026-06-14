#include <MazeBuilder/state.h>

#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/runtime_stack.h>

using namespace mazes;

void state::request_stack_push(ID state_id) noexcept
{
    _runtime_stack->push_state(state_id);
}

void state::request_stack_pop() noexcept
{
    _runtime_stack->pop_state();
}

void state::request_stack_clear() noexcept
{
    _runtime_stack->clear_states();
}

const runtime_app::context &state::get_context() const noexcept { return ctx; }

runtime_stack &state::get_stack() const noexcept { return *_runtime_stack; }
