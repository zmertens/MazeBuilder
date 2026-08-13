#ifndef PIXELS_CREATE_STATE_H
#define PIXELS_CREATE_STATE_H

#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/state.h>

#include <optional>
#include <string>
#include <string_view>

/// @namespace mazes
/// @file pixels_create_state.h
namespace mazes
{
    class configurator;
    class randomizer;
    class runtime_stack;
    struct context;

    /// @brief State for creating a maze using the pixels algorithm
    class pixels_create_state final : public create_contract, public state
    {
    public:
        /// @brief Construct a new pixels maze creation state
        /// @param ctx The context of the runtime application
        /// @param stack The runtime stack
        explicit pixels_create_state(const runtime_app::context &ctx, runtime_stack *stack);

        /// @brief Creates a maze using the specified algorithm and parameters
        /// @param a
        /// @param rows
        /// @param cols
        /// @param levels
        /// @param rng
        /// @return
        [[nodiscard]] std::string_view create(const configurator &config, randomizer &rng) noexcept override;

        void draw() const noexcept override;

        /// @brief Updates the state of the maze creation process
        /// @param delta_time Time elapsed since the last update
        /// @return True if the update was successful, false otherwise
        [[nodiscard]] bool update(double delta_time) noexcept override;

    private:
        grid_manager *grid_mapper;
        processed_text_manager *processed_text_mapper;
        grid_identifier current_grid_id{grid_identifier::BASIC};
        std::string m_result;
        int m_image_width{0};
        int m_image_height{0};
        std::optional<unsigned long long> m_palette_seed;
    };
} // namespace mazes

#endif // PIXELS_CREATE_STATE_H
