#ifndef PARSING_STATE_H
#define PARSING_STATE_H

#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/state.h>

#include <any>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/// @namespace mazes
/// @file parsing_state.h
namespace mazes
{
    class args;
    class randomizer;
    class runtime_stack;
    struct context;

    /// @brief State for parsing arguments
    class parsing_state final : public create_contract, public state
    {
    public:
        explicit parsing_state(const runtime_app::context& ctx, runtime_stack* rs);

        /// @brief Normalizes the raw input into a canonical, space-separated string
        /// @details Collapses whitespace and splits "--opt=value" into "--opt value"
        /// @return The canonical string as std::string, or an empty std::any if the input is empty
        [[nodiscard]] std::any create(const args &a, randomizer &rng) noexcept override;

        void draw() const noexcept override;

        /// @brief Updates the state of the parsing process
        /// @param rng Randomizer to use for the update
        /// @param delta_time Time elapsed since the last update
        /// @return True if the update was successful, false otherwise
        [[nodiscard]] bool update(randomizer &rng, double delta_time) noexcept override;
    private:
        args_manager* args_mapper;
    };
} // namespace mazes

#endif // PARSING_STATE_H
