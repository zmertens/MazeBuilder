#ifndef PARSING_STATE_H
#define PARSING_STATE_H

#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/state.h>

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class randomizer;
class runtime_stack;
struct context;

/// @namespace mazes
/// @file parsing_state.h
namespace mazes
{
    using arg_map = std::unordered_map<std::string, std::string>;

    /// @brief Outcome of parse_args; maps is empty whenever ok is false
    struct parse_result
    {
        bool ok{false};
        std::vector<arg_map> maps;
        std::string error;
    };

    /// @brief Parses tokens by running them through the parse_sub_state chain
    /// @param tokens Already-split command-line tokens
    /// @param has_program_name Whether tokens[0] is the program name
    /// @return One map per argument set (a JSON array yields several)
    [[nodiscard]] parse_result parse_args(std::span<const std::string> tokens, bool has_program_name) noexcept;

    /// @brief State for parsing arguments
    class parsing_state final : public state
    {
    public:
        explicit parsing_state(const runtime_app::context& ctx, runtime_stack* rs);

        /// @brief Converts a string of arguments into an args object
        /// @param arguments The string of arguments to convert
        /// @return An optional args object if the conversion was successful, std::nullopt otherwise
        [[nodiscard]] std::optional<args> convert(std::string_view arguments) const noexcept;

        void draw() const noexcept override;

        /// @brief Updates the state of the parsing process
        /// @param rng Randomizer to use for the update
        /// @param delta_time Time elapsed since the last update
        /// @return True if the update was successful, false otherwise
        [[nodiscard]] bool update(randomizer &rng, double delta_time) noexcept override;

    private:
        args_manager* args_mapper;
        processed_text_manager* processed_text_mapper;
    };
} // namespace mazes

#endif // PARSING_STATE_H
