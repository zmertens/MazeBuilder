#ifndef STATE_H
#define STATE_H

#include <MazeBuilder/runtime_app.h>

#include <functional>
#include <optional>

/// @file state.h
/// @namespace mazes
namespace mazes
{
    class args;
    class runtime_stack;

    /// @brief struct representing a state in the runtime stack
    struct state
    {
        enum class ID : unsigned int
        {
            BTING = 0,
            DFSING = 1,
            EMPTY = 2,
            LOADING = 3,
            PARSING = 4,
            PIXELIZING = 5,
            SIDEWINDERING = 6,
            STRINGIFYING = 7,
            WAVEFRONT_OBJECTIFYING = 8,
            TOTAL = 9
        };

        explicit state(const runtime_app::context& c, runtime_stack* rs)
            : ctx(c), _runtime_stack(rs)
        {
        }

        virtual ~state() = default;

        virtual void draw() const noexcept = 0;

        /// @brief Update the state with the given arguments and elapsed time
        /// @param args The optional arguments to pass to the state's update function
        /// @param delta_time The elapsed time since the last update
        /// @return True if the state should continue updating, false otherwise
        virtual bool update(const std::optional<args>& args, double delta_time) noexcept = 0;

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
