#ifndef BT_MAZE_CREATE_STATE
#define BT_MAZE_CREATE_STATE

#include <MazeBuilder/algos.h>
#include <MazeBuilder/link_maze_and_create_state.h>

#include <string_view>

class configurator;
class randomizer;
class runtime_stack;

/// @namespace mazes
/// @file bt_maze_create_state.h
namespace mazes
{
    /// @brief State for creating a maze using the binary tree algorithm
    class bt_maze_create_state final : public link_maze_and_create_state
    {
    public:
        /// @brief Construct a new binary tree maze creation state
        /// @param ctx The runtime context
        /// @param stack The runtime stack to allow pushing/popping states
        explicit bt_maze_create_state(const runtime_app::context& ctx, runtime_stack* stack);

        /// @brief Create a binary tree maze with the given parameters
        /// @param config The maze configuration
        /// @param rng The randomizer for the algorithm
        /// @return A string view with the result message
        std::string_view create(const configurator& config, randomizer& rng) noexcept override;

    private:
        /// @brief Get the algorithm ID for the binary tree algorithm
        [[nodiscard]] algo get_algo_id() const noexcept override;

        /// @brief Implementation of the binary tree maze creation algorithm
        /// @param rows The number of rows in the maze
        /// @param cols The number of columns in the maze
        /// @param levels The number of levels in the maze
        /// @param rng The randomizer for the algorithm
        /// @return A string view with the result message
        std::string_view create_bt_maze(unsigned int rows, unsigned int cols, unsigned int levels,
            randomizer& rng) noexcept;
    };
} // namespace mazes

#endif // BT_MAZE_CREATE_STATE
