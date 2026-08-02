#ifndef LOADING_STATE_H
#define LOADING_STATE_H

#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/state.h>

#include <mutex>
#include <optional>

/// @file loading_state.h
/// @namespace mazes
namespace mazes
{
    class args;
    class runtime_stack;

    /// @brief State for loading and pre-generating mazes
    class loading_state final : public state
    {
    public:
        // constructor
        explicit loading_state(const runtime_app::context& ctx, runtime_stack* rs);

        void draw() const noexcept override;

        /// @brief Updates the state of the loading process
        /// @param args Optional arguments for the update
        /// @param delta_time Time elapsed since the last update
        /// @return True if the update was successful, false otherwise
        bool update(const std::optional<args>& args, double delta_time) noexcept override;

    private:
        /// @brief Loads the necessary resources for the loading state
        /// @param args Optional arguments for resource loading
        void load_resources(const std::optional<args>& args) const noexcept;

        grid_manager* grid_mapper;
        processed_text_manager* processed_text_mapper;

        std::once_flag resource_loaded_flag;
        bool has_finished{false};
    };
} // namespace mazes

#endif // LOADING_STATE_H
