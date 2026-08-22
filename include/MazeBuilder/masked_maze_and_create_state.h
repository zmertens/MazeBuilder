#ifndef MASKED_MAZE_AND_CREATE_STATE_H
#define MASKED_MAZE_AND_CREATE_STATE_H

#include <MazeBuilder/algos.h>
#include <MazeBuilder/link_maze_and_create_state.h>

#include <string_view>

class configurator;
class randomizer;
class runtime_stack;

/// @namespace mazes
/// @file masked_maze_and_create_state.h
namespace mazes
{
    /// @brief State for creating a maze with a mask applied
    /// @details This state loads a mask from file and creates a maze using the specified
    ///          algorithm (binary_tree, sidewinder, or dfs) on a masked_grid.
    ///          Masked cells are excluded from the maze generation process.
    class masked_maze_and_create_state final : public link_maze_and_create_state
    {
    public:
        /// @brief Construct a new masked maze creation state
        /// @param ctx The runtime context
        /// @param stack The runtime stack to allow pushing/popping states
        explicit masked_maze_and_create_state(const runtime_app::context& ctx, runtime_stack* stack);

        /// @brief Create a masked maze with the given parameters
        /// @param config The maze configuration
        /// @param rng The randomizer for the algorithm
        /// @return A string view with the result message
        std::string_view create(const configurator& config, randomizer& rng) noexcept override;

    private:
        /// @brief Get the algorithm ID for the masked maze algorithm
        [[nodiscard]] algo get_algo_id() const noexcept override;

        /// @brief Implementation of the masked maze creation
        /// @param mask_file Path to the mask file
        /// @param algorithm The maze generation algorithm to use
        /// @param rng The randomizer for the algorithm
        /// @return A string view with the result message
        std::string_view create_masked_maze(const std::string& mask_file, algo algorithm,
            randomizer& rng) noexcept;
    };
} // namespace mazes

#endif // MASKED_MAZE_AND_CREATE_STATE_H
