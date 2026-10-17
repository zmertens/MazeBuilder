#ifndef PIXELS_CREATE_STATE_H
#define PIXELS_CREATE_STATE_H

#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/write_to_output_state.h>

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

    /// @brief State for creating a maze using the pixels algorithm
    class pixels_create_state final : public create_contract, public write_to_output_state
    {
    public:
        /// @brief Construct a new pixels maze creation state
        /// @param ctx The context of the runtime application
        /// @param stack The runtime stack
        explicit pixels_create_state(const runtime_app::context &ctx, mazes::runtime_stack *stack);

        /// @brief Creates a maze using the specified algorithm and parameters
        /// @param config The maze configuration
        /// @param rng The randomizer for the algorithm
        /// @return A string view with the result message
        [[nodiscard]] std::any create(const args &a, randomizer &rng) noexcept override;

    protected:
        /// @brief Write pixel data to image file in appropriate format
        [[nodiscard]] bool write_output(const std::string &output_target, const std::string &output_content) noexcept override;

    private:
        int m_image_width{0};
        int m_image_height{0};
        std::optional<unsigned long long> m_palette_seed;
    };
} // namespace mazes

#endif // PIXELS_CREATE_STATE_H
