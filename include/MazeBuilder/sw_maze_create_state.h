#ifndef SW_MAZE_CREATE_STATE_H
#define SW_MAZE_CREATE_STATE_H

#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/state.h>

#include <string_view>
#include <string>

/// @file sw_maze_create_state.h
/// @namespace mazes
namespace mazes
{
    class configurator;
    class randomizer;
    class runtime_stack;
    struct context;

    /// @brief State for creating a maze using the sidewinder algorithm
    class sw_maze_create_state final : public create_contract, public state
    {
    public:
        explicit sw_maze_create_state(const runtime_app::context &ctx, runtime_stack *rs);

        std::string_view create(const configurator &config, randomizer &rng) noexcept override;

        void draw() const noexcept override;

        /// @brief Update the state with the given arguments and elapsed time
        /// @param delta_time The elapsed time since the last update
        /// @return True if the state should continue updating, false otherwise
        bool update(double delta_time) noexcept override;

    private:
        /// @brief Implementation of the sidewinder maze creation algorithm
        /// @param rows
        /// @param cols
        /// @param levels
        /// @param rng
        /// @return
        std::string_view create_sw_maze(unsigned int rows, unsigned int cols, unsigned int levels,
                                        randomizer &rng) noexcept;

        grid_manager *grid_mapper;
        processed_text_manager *processed_text_mapper;
        grid_identifier current_grid_id;
        bool m_use_distances;
        int m_distances_start;
        int m_distances_end;
        std::string m_result;
    };
} // namespace mazes

#endif // SW_MAZE_CREATE_STATE_H
