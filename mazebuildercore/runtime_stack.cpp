#include <MazeBuilder/runtime_stack.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/runtime_app.h>

#include <ranges>

// Fully qualify runtime_stack to avoid ambiguity
mazes::runtime_stack::runtime_stack(const mazes::runtime_app::context& ctx)
    : runtime_context(ctx)
{
}

void mazes::runtime_stack::push_state(state::ID state_id) noexcept
{
    m_pending.emplace_back(Operation::PUSH, state_id);
}

void mazes::runtime_stack::pop_state() noexcept
{
    m_pending.emplace_back(Operation::POP);
}

void mazes::runtime_stack::clear_states() noexcept
{
    m_pending.emplace_back(Operation::CLEAR);
}

void mazes::runtime_stack::apply_pending_changes() noexcept
{
    for (const auto& change : m_pending)
    {
        switch (change.action)
        {
        case Operation::PUSH:
            try
            {
                m_states.push_back(create_state(change.state_id));
            } catch (...)
            {
            }
            break;

        case Operation::POP:
            if (!m_states.empty())
            {
                m_states.pop_back();
            }
            break;

        case Operation::CLEAR:
            m_states.clear();
            break;
        }
    }

    m_pending.clear();
}

bool mazes::runtime_stack::is_empty() const noexcept
{
    return m_states.empty() && m_pending.empty();
}

std::size_t mazes::runtime_stack::count() const noexcept
{
    return m_states.size();
}

void mazes::runtime_stack::visit_states(double elapsed) noexcept
{
    apply_pending_changes();

    while (!m_states.empty())
    {
        auto* top_state = m_states.back().get();
        if (top_state == nullptr)
        {
            m_states.pop_back();
            continue;
        }

        const bool should_continue = top_state->update(elapsed);
        apply_pending_changes();

        if (!should_continue)
        {
            break;
        }
    }
}
