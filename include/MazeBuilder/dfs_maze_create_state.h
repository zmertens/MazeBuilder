#ifndef DFS_MAZE_CREATE_STATE_H
#define DFS_MAZE_CREATE_STATE_H

#include <MazeBuilder/algos.h>
#include <MazeBuilder/link_maze_and_create_state.h>

/// @namespace mazes
/// @file dfs_maze_create_state.h
namespace mazes
{
    class args;
    class randomizer;
    class runtime_stack;
    /// @brief State for creating a maze using the depth-first search algorithm
    class dfs_maze_create_state final : public link_maze_and_create_state
    {
    public:
        /// @brief Construct a new depth-first search maze creation state
        /// @param ctx The runtime context
        /// @param stack The runtime stack to allow pushing/popping states
        explicit dfs_maze_create_state(const runtime_app::context &ctx, runtime_stack *stack);

        /// @brief Create a maze using the depth-first search algorithm
        /// @param config The maze configuration
        /// @param rng The randomizer for the algorithm
        /// @return A string view with the result message
        std::any create(const args &a, randomizer &rng) noexcept override;

    private:
        /// @brief Get the algorithm ID for the depth-first search algorithm
        [[nodiscard]] algo get_algo_id() const noexcept override;
    };
} // namespace mazes

#endif // DFS_MAZE_CREATE_STATE_H
