#ifndef WRITE_TO_OUTPUT_STATE_H
#define WRITE_TO_OUTPUT_STATE_H

#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/state.h>

#include <string>
#include <string_view>

class configurator;
class randomizer;
class runtime_stack;

/// @namespace mazes
/// @file write_to_output_state.h
namespace mazes
{
    /// @brief Base state for creating output from mazes (pixels, stringify, OBJ)
    /// This state encapsulates the common setup, creation, and output-writing logic
    /// for states that generate maze output in various formats
    class write_to_output_state : public create_contract, public state
    {
    public:
        /// @brief Construct a new write to output state
        /// @param ctx The runtime context
        /// @param stack The runtime stack to allow pushing/popping states
        explicit write_to_output_state(const runtime_app::context& ctx, runtime_stack* stack);

        virtual ~write_to_output_state() = default;

        /// @brief Update the state with the given arguments and delta time
        /// Common logic for parsing arguments, creating output, and writing to file
        /// @param delta_time Time elapsed since the last update
        /// @return False to indicate the state has completed (pops itself from the stack)
        bool update(double delta_time) noexcept override final;

        void draw() const noexcept override;

    protected:
        /// @brief Create the output using algorithm-specific logic
        /// @param config The configuration containing maze parameters
        /// @param rng The randomizer for the algorithm
        /// @return A string view with the result message/content
        [[nodiscard]] virtual std::string_view create(const configurator& config, randomizer& rng) noexcept = 0;

        /// @brief Write the output to the target destination
        /// Subclasses should override this to handle format-specific writing
        /// @param output_target The target file path or "stdout"
        /// @param output_content The content to write
        /// @return True if write was successful, false otherwise
        [[nodiscard]] virtual bool write_output(const std::string& output_target, const std::string& output_content) noexcept = 0;

        grid_manager* grid_mapper;
        processed_text_manager* processed_text_mapper;
        grid_identifier current_grid_id;
        std::string m_result;
    };
} // namespace mazes

#endif // WRITE_TO_OUTPUT_STATE_H
