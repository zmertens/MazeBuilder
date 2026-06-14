#ifndef PARSING_STATE_H
#define PARSING_STATE_H

#include <MazeBuilder/convert_contract.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/state.h>

#include <optional>
#include <string_view>

/// @namespace mazes
/// @file parsing_state.h
namespace mazes
{

    class args;
    class runtime_stack;
    struct context;

    /// @brief State for parsing arguments
    class parsing_state final : public convert, public state
    {
    public:
        explicit parsing_state(const runtime_app::context &ctx, runtime_stack *stack);

        /// @brief Converts a string of arguments into an args object
        /// @param arguments The string of arguments to convert
        /// @return An optional args object if the conversion was successful, std::nullopt otherwise
        std::optional<args> convert(std::string_view arguments) const noexcept override;

        void draw() const noexcept override;

        /// @brief Updates the state of the parsing process
        /// @param args Optional arguments for the update
        /// @param delta_time Time elapsed since the last update
        /// @return True if the update was successful, false otherwise
        bool update(const std::optional<args> &args, double delta_time) noexcept override;

    private:
        grid_manager *grid_mapper;
        processed_text_manager *processed_text_mapper;
    };

} // namespace mazes

#endif // PARSING_STATE_H
