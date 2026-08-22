#ifndef STATE_H
#define STATE_H

#include <MazeBuilder/runtime_app.h>

#include <functional>
#include <optional>

class runtime_stack;

/// @file state.h
/// @namespace mazes
namespace mazes
{
    /// @brief struct representing a state in the runtime stack
    struct state
    {
        enum class ID : unsigned int
        {
            EMPTY = 0,
            LINK_WITH_BINARY_TREE = 1,
            LINK_WITH_DFS = 2,
            LINK_WITH_SIDEWINDER = 3,
            LOAD = 4,
            PARSE = 5,
            WRITE_TO_IMAGE = 6,
            WRITE_TO_STRING = 7,
            WRITE_TO_WF_OBJ = 8,
            LINK_WITH_MASKED = 9,
            TOTAL = 10
        };

        explicit state(const runtime_app::context& c, runtime_stack* rs)
            : ctx(c), _runtime_stack(rs)
        {
        }

        virtual ~state() = default;

        virtual void draw() const noexcept = 0;

        /// @brief Update the state with the elapsed time
        /// @param delta_time The elapsed time since the last update
        /// @return True if the state should continue updating, false otherwise
        virtual bool update(double delta_time) noexcept = 0;

    protected:
        void request_stack_push(ID state_id) const noexcept;

        void request_stack_pop() const noexcept;

        void request_stack_clear() const noexcept;

        [[nodiscard]] const runtime_app::context& get_context() const noexcept;

        [[nodiscard]] runtime_stack& get_stack() const noexcept;

    private:
        std::reference_wrapper<const runtime_app::context> ctx;
        runtime_stack* _runtime_stack;
    };
} // namespace mazes

#endif // STATE_H
