#ifndef LINK_MAZE_AND_CREATE_STATE_H
#define LINK_MAZE_AND_CREATE_STATE_H

#include <MazeBuilder/algos.h>
#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/state.h>

#include <string_view>
#include <string>

class configurator;
class randomizer;
class runtime_stack;

/// @namespace mazes
/// @file link_maze_and_create_state.h
namespace mazes
{
    /// @brief Base state for creating mazes using algorithms that link cells
    /// This state encapsulates the common setup and teardown logic for maze-generating
    /// algorithms like binary tree, depth-first search, and sidewinder
    class link_maze_and_create_state : public create_contract, public state
    {
    public:
        /// @brief Construct a new link maze and create state
        /// @param ctx The runtime context
        /// @param stack The runtime stack to allow pushing/popping states
        explicit link_maze_and_create_state(const runtime_app::context& ctx, runtime_stack* stack);

        virtual ~link_maze_and_create_state() = default;

        /// @brief Update the state with the given arguments and delta time
        /// Common logic for parsing arguments, setting up configuration, and handling output
        /// @param delta_time Time elapsed since the last update
        /// @return False to indicate the state has completed (pops itself from the stack)
        bool update(double delta_time) noexcept override final;

        void draw() const noexcept override;

    protected:
        /// @brief Get the algorithm ID for this maze creation algorithm
        /// @return The algorithm ID to use in the configurator
        [[nodiscard]] virtual algo get_algo_id() const noexcept = 0;

        /// @brief Create the maze using algorithm-specific logic
        /// @param config The configuration containing maze parameters
        /// @param rng The randomizer for the algorithm
        /// @return A string view with the result message
        [[nodiscard]] virtual std::string_view create(const configurator& config, randomizer& rng) noexcept = 0;

        grid_manager* grid_mapper;
        processed_text_manager* processed_text_mapper;
        grid_identifier current_grid_id;
        bool m_use_distances;
        int m_distances_start;
        int m_distances_end;
        std::string m_result;

    private:
        /// @brief Common update logic for all link-based maze creation algorithms
        void common_update(const configurator& cfg, randomizer& rng) noexcept;
    };
} // namespace mazes

#endif // LINK_MAZE_AND_CREATE_STATE_H
