#ifndef RUNTIME_STACK_H
#define RUNTIME_STACK_H

#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/state.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/// @file runtime_stack.h
/// @namespace mazes
namespace mazes
{
    /// @brief Class representing the runtime stack of states in the maze builder application
    class runtime_stack final
    {
    public:
        enum class operation : unsigned int
        {
            PUSH = 0,
            POP = 1,
            CLEAR = 2
        };

        explicit runtime_stack(const runtime_app::context& ctx);

        // Stack operations (deferred — applied by apply_pending_changes)
        void push_state(state::ID state_id) noexcept;
        void pop_state() noexcept;
        void clear_states() noexcept;

        /// @brief Visit each state top-to-bottom, calling update(args, elapsed) until one returns false.
        ///        Applies pending stack changes after the pass.
        /// @param args The optional arguments to pass to each state's update function
        /// @param elapsed The elapsed time since the last update
        void visit_states(const std::optional<args>& args, double elapsed) noexcept;

        /// @brief Checks if the runtime stack is empty
        /// @return True if the stack is empty, false otherwise
        [[nodiscard]] bool is_empty() const noexcept;

        /// @brief Register a factory for a concrete state type.
        ///        The factory is called with (runtime_context, this).
        template <typename T>
        void register_state(state::ID state_id)
        {
            m_factories.insert_or_assign(state_id, [this]()
            {
                return std::make_unique<T>(runtime_context, this);
            });
        }

        /// @brief Find the topmost state that matches the pointer type T.
        template <typename Pointer>
        [[nodiscard]] Pointer peek_state() const noexcept
        {
            auto reversed = m_states | std::views::reverse;

            auto it = std::ranges::find_if(reversed, [](const auto& sp)
            {
                return dynamic_cast<Pointer>(sp.get()) != nullptr;
            });

            if (it != std::ranges::cend(reversed))
            {
                return dynamic_cast<Pointer>(it->get());
            }

            return nullptr;
        }

    private:
        struct pending_change
        {
            explicit pending_change(operation action, state::ID id = state::ID::TOTAL)
                : action(action), state_id(id)
            {
            }

            operation action;
            state::ID state_id;
        };

        struct state_id_hash
        {
            std::size_t operator()(state::ID id) const noexcept
            {
                return std::hash<unsigned int>{}(static_cast<unsigned int>(id));
            }
        };

        // Apply all pending push/pop/clear operations
        void apply_pending_changes() noexcept;

        [[nodiscard]] std::unique_ptr<state> create_state(state::ID state_id)
        {
            if (const auto& found = m_factories.find(state_id); found != m_factories.cend())
            {
                return found->second();
            }

            throw std::runtime_error("runtime_stack::create_state - No factory for state ID: " +
                std::to_string(static_cast<unsigned int>(state_id)));
        }

        std::vector<std::unique_ptr<state>> m_states;
        std::vector<pending_change> m_pending;
        runtime_app::context runtime_context;
        std::unordered_map<state::ID,
                           std::function<std::unique_ptr<state>()>,
                           state_id_hash>
        m_factories;
    };
} // namespace mazes

#endif // RUNTIME_STACK_H
