#include <MazeBuilder/runtime_stack.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/runtime_app.h>

#include <ranges>
#include <stdexcept>

using namespace mazes;

runtime_stack::runtime_stack(runtime_app::context ctx)
    : runtime_context(ctx)
{
}

void runtime_stack::push_state(state::ID state_id) noexcept
{
    m_pending.emplace_back(operation::PUSH, state_id);
}

void runtime_stack::pop_state() noexcept
{
    m_pending.emplace_back(operation::POP);
}

void runtime_stack::clear_states() noexcept
{
    m_pending.emplace_back(operation::CLEAR);
}

void runtime_stack::apply_pending_changes() noexcept
{
    for (const auto &change : m_pending)
    {
        switch (change.action)
        {
        case operation::PUSH:
            try
            {
                m_states.push_back(create_state(change.state_id));
            }
            catch (...)
            {
            }
            break;

        case operation::POP:
            if (!m_states.empty())
            {
                m_states.pop_back();
            }
            break;

        case operation::CLEAR:
            m_states.clear();
            break;
        }
    }

    m_pending.clear();
}

bool runtime_stack::is_empty() const noexcept
{
    return m_states.empty() && m_pending.empty();
}

void runtime_stack::visit_states(const std::optional<args> &args, double elapsed) noexcept
{
    std::ranges::for_each(m_states | std::views::reverse, [&args, elapsed](const auto &state_ptr)
                          {
        if (!state_ptr->update(args, elapsed))
        {
            // If state does not update - do not change the stack - try to process next iteration
            return;
        } });

    apply_pending_changes();
}