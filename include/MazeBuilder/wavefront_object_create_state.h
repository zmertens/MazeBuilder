#ifndef WAVEFRONT_OBJECT_CREATE_STATE_H
#define WAVEFRONT_OBJECT_CREATE_STATE_H

#include <MazeBuilder/write_to_output_state.h>

#include <string>
#include <string_view>

class configurator;
class randomizer;
class runtime_stack;

/// @file wavefront_object_create_state.h
/// @namespace mazes
namespace mazes
{
    /// @brief State for creating a maze using the Wavefront Object algorithm
    class wavefront_object_create_state final : public write_to_output_state
    {
    public:
        explicit wavefront_object_create_state(const runtime_app::context& ctx, runtime_stack* rs);

        /// @brief Create a maze using the Wavefront Object algorithm
        /// @param config The configurator containing the maze configuration
        /// @param rng The randomizer to use for generating random values
        /// @return A string view with the OBJ content
        std::string_view create(const configurator& config, randomizer& rng) noexcept override;

    private:
        /// @brief Write OBJ content to output file
        [[nodiscard]] bool write_output(const std::string& output_target, const std::string& output_content) noexcept override;
    };
} // namespace mazes

#endif // WAVEFRONT_OBJECT_CREATE_STATE_H
