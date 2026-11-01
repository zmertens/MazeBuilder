#ifndef LOADING_STATE_H
#define LOADING_STATE_H

#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/state.h>

class randomizer;
class runtime_stack;

/// @file loading_state.h
/// @namespace mazes
namespace mazes
{
    /// @brief State for loading and pre-generating mazes
    class loading_state final : public state
    {
    public:
        // constructor
        explicit loading_state(const runtime_app::context &ctx, runtime_stack *rs);

        void draw() const noexcept override;

        /// @brief Updates the state of the loading process
        /// @param delta_time Time elapsed since the last update
        /// @return True if the update was successful, false otherwise
        bool update(randomizer &rng, double delta_time) noexcept override;

    private:
        /// @brief Loads the necessary resources for the loading state
        void load_resources() const noexcept;

        grid_manager *grid_mapper;
        processed_text_manager *processed_text_mapper;
        args_manager *args_mapper;

        bool has_finished{false};
    };
} // namespace mazes

#endif // LOADING_STATE_H
