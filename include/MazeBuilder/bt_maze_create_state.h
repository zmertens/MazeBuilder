#ifndef BT_MAZE_CREATE_STATE
#define BT_MAZE_CREATE_STATE

#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/state.h>

#include <string_view>
#include <string>

/// @namespace mazes
/// @file bt_maze_create_state.h
namespace mazes
{
    class args;
    class configurator;
    class randomizer;
    class runtime_stack;
    struct context;

    /// @brief State for creating a maze using the binary tree algorithm
    class bt_maze_create_state final : public create_contract, public state
    {
    public:
        /// @brief Construct a new binary tree maze creation state
        /// @param ctx The runtime context
        /// @param stack The runtime stack to allow pushing/popping states
        explicit bt_maze_create_state(const runtime_app::context& ctx, runtime_stack* stack);

        /// @brief Create a binary tree maze with the given parameters
        /// @param a
        /// @param rows
        /// @param cols
        /// @param levels
        /// @param rng
        /// @return
        std::string_view create(const configurator& config, randomizer& rng) noexcept override;

        /// @brief @TODO -> pre-print mazes as a visualization technique
        void draw() const noexcept override;

        /// @brief Update the state with the given arguments and delta time
        /// @param args Optional arguments for the update
        /// @param delta_time Time elapsed since the last update
        /// @return True if the state was updated successfully, false otherwise
        bool update(const std::optional<args>& args, double delta_time) noexcept override;

    private:
        /// @brief Implementation of the binary tree maze creation algorithm
        /// @param rows
        /// @param cols
        /// @param levels
        /// @param rng
        /// @return
        std::string_view create_bt_maze(unsigned int rows, unsigned int cols, unsigned int levels,
                                        randomizer& rng) noexcept;

        grid_manager* grid_mapper;
        processed_text_manager* processed_text_mapper;
        grid_identifier m_grid_id;
        bool m_use_distances;
        int m_distances_start;
        int m_distances_end;
        std::string m_result;
    };
} // namespace mazes

#endif // BT_MAZE_CREATE_STATE
