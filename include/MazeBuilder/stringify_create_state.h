#ifndef STRINGIFY_CREATE_STATE_H
#define STRINGIFY_CREATE_STATE_H

#include <MazeBuilder/write_to_output_state.h>

#include <string>
#include <string_view>

/// @file stringify_create_state.h
/// @namespace mazes
namespace mazes
{
    class configurator;
    class randomizer;
    class runtime_stack;

    /// @brief State for creating a maze using the stringify algorithm
    class stringify_create_state final : public write_to_output_state
    {
    public:
        explicit stringify_create_state(const runtime_app::context &ctx, runtime_stack *rs);

        std::string_view create(const configurator &config, randomizer &rng) noexcept override;

    private:
        /// @brief Write ASCII maze string to output target or stdout
        [[nodiscard]] bool write_output(const std::string& output_target, const std::string& output_content) noexcept override;
    };
} // namespace mazes

#endif // STRINGIFY_CREATE_STATE_H
