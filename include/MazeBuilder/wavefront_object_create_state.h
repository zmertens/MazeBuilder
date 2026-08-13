#ifndef WAVEFRONT_OBJECT_CREATE_STATE_H
#define WAVEFRONT_OBJECT_CREATE_STATE_H

#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/state.h>

#include <string>
#include <string_view>

/// @file wavefront_object_create_state.h
/// @namespace mazes
namespace mazes
{
    class configurator;
    class randomizer;
    class runtime_stack;
    struct context;

    /// @brief State for creating a maze using the Wavefront Object algorithm
    class wavefront_object_create_state final : public create_contract, public state
    {
    public:
        explicit wavefront_object_create_state(const runtime_app::context &ctx, runtime_stack *rs);

        /// @brief Create a maze using the Wavefront Object algorithm
        /// @param config The configurator containing the maze configuration
        /// @param rng The randomizer to use for generating random values
        /// @return
        std::string_view create(const configurator &config, randomizer &rng) noexcept override;

        void draw() const noexcept override;

        /// @brief Update the state with the elapsed time
        /// @param delta_time The elapsed time since the last update
        /// @return True if the state should continue updating, false otherwise
        bool update(double delta_time) noexcept override;

    private:
        grid_manager *grid_mapper;
        processed_text_manager *processed_text_mapper;
        grid_identifier current_grid_id{grid_identifier::BASIC};
        std::string m_result;
    };
} // namespace mazes

#endif // WAVEFRONT_OBJECT_CREATE_STATE_H
