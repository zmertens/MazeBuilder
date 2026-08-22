#ifndef PIXELS_CREATE_STATE_H
#define PIXELS_CREATE_STATE_H

#include <MazeBuilder/write_to_output_state.h>

#include <optional>
#include <string>
#include <string_view>

class configurator;
class randomizer;
class runtime_stack;

/// @namespace mazes
/// @file pixels_create_state.h
namespace mazes
{
    /// @brief State for creating a maze using the pixels algorithm
    class pixels_create_state final : public write_to_output_state
    {
    public:
        /// @brief Construct a new pixels maze creation state
        /// @param ctx The context of the runtime application
        /// @param stack The runtime stack
        explicit pixels_create_state(const runtime_app::context &ctx, runtime_stack *stack);

        /// @brief Creates a maze using the specified algorithm and parameters
        /// @param config The maze configuration
        /// @param rng The randomizer for the algorithm
        /// @return A string view with the result message
        [[nodiscard]] std::string_view create(const configurator &config, randomizer &rng) noexcept override;

    private:
        /// @brief Write pixel data to image file in appropriate format
        [[nodiscard]] bool write_output(const std::string& output_target, const std::string& output_content) noexcept override;

        int m_image_width{0};
        int m_image_height{0};
        std::optional<unsigned long long> m_palette_seed;
    };
} // namespace mazes

#endif // PIXELS_CREATE_STATE_H
