#ifndef DFS_MAZE_CREATE_STATE_H
#define DFS_MAZE_CREATE_STATE_H

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/state.h>

#include <string_view>
#include <string>

/// @namespace mazes
/// @file dfs_maze_create_state.h
namespace mazes
{
    class args;
    class randomizer;
    class runtime_stack;
    struct context;

    /// @brief State for creating a maze using the depth-first search algorithm
    class dfs_maze_create_state final : public create_contract, public state
    {
    public:
        // Constructor
        explicit dfs_maze_create_state(const runtime_app::context& ctx, runtime_stack* stack);

        /// @brief Create a maze using the depth-first search algorithm
        /// @param a
        /// @param rows
        /// @param cols
        /// @param levels
        /// @param rng
        /// @return
        std::string_view create(const configurator& config, randomizer& rng) noexcept override;

        void draw() const noexcept override;

        /// @brief Update the state of the maze creation
        /// @param args Optional arguments for the update
        /// @param delta_time Time elapsed since the last update
        /// @return True if the update was successful, false otherwise
        bool update(const std::optional<args>& args, double delta_time) noexcept override;

    private:
        /// @brief Implementation of the depth-first search maze creation algorithm
        /// @param rows
        /// @param cols
        /// @param levels
        /// @param rng
        /// @return
        std::string_view create_dfs_maze(unsigned int rows, unsigned int cols, unsigned int levels,
                                         randomizer& rng) noexcept;

        grid_manager* grid_mapper;
        processed_text_manager* processed_text_mapper;
        grid_identifier m_grid_id{grid_identifier::BASIC};
        bool m_use_distances{false};
        int m_distances_start{configurator::DEFAULT_DISTANCES_START};
        int m_distances_end{configurator::DEFAULT_DISTANCES_END};
        std::string m_result;
    };
} // namespace mazes

#endif // DFS_MAZE_CREATE_STATE_H
