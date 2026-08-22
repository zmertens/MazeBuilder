#ifndef SW_MAZE_CREATE_STATE_H
#define SW_MAZE_CREATE_STATE_H

#include <MazeBuilder/algos.h>
#include <MazeBuilder/link_maze_and_create_state.h>

#include <string_view>

class randomizer;
class runtime_stack;

/// @file sw_maze_create_state.h
/// @namespace mazes
namespace mazes
{
    /// @brief State for creating a maze using the sidewinder algorithm
    class sw_maze_create_state final : public link_maze_and_create_state
    {
    public:
        /// @brief Construct a new sidewinder maze creation state
        /// @param ctx The runtime context
        /// @param rs The runtime stack to allow pushing/popping states
        explicit sw_maze_create_state(const runtime_app::context& ctx, runtime_stack* rs);

        /// @brief Create a maze using the sidewinder algorithm
        /// @param config The maze configuration
        /// @param rng The randomizer for the algorithm
        /// @return A string view with the result message
        std::string_view create(const configurator& config, randomizer& rng) noexcept override;

    private:
        /// @brief Get the algorithm ID for the sidewinder algorithm
        [[nodiscard]] algo get_algo_id() const noexcept override;

        /// @brief Implementation of the sidewinder maze creation algorithm
        /// @param rows The number of rows in the maze
        /// @param cols The number of columns in the maze
        /// @param levels The number of levels in the maze
        /// @param rng The randomizer for the algorithm
        /// @return A string view with the result message
        std::string_view create_sw_maze(unsigned int rows, unsigned int cols, unsigned int levels,
            randomizer& rng) noexcept;
    };
} // namespace mazes

#endif // SW_MAZE_CREATE_STATE_H
