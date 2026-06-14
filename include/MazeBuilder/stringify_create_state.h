#ifndef STRINGIFY_CREATE_STATE
#define STRINGIFY_CREATE_STATE

#include <MazeBuilder/algos.h>
#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/state.h>

#include <string_view>

/// @file stringify_create_state.h
/// @namespace mazes
namespace mazes
{
    class args;
    class randomizer;
    class runtime_stack;
    struct context;

    /// @brief State for creating a maze using the stringify algorithm
    class stringify_create_state final : public create_contract, public state
    {
    public:
        explicit stringify_create_state(const runtime_app::context &ctx, runtime_stack *stack);

        virtual std::string_view create(algo a, unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept override;

        void draw() const noexcept override;

        /// @brief Update the state with the given arguments and elapsed time
        /// @param args The optional arguments to pass to the state's update function
        /// @param delta_time The elapsed time since the last update
        /// @return True if the state should continue updating, false otherwise
        bool update(const std::optional<args> &args, double delta_time) noexcept override;

    private:
        /// @brief Implementation of the stringify maze creation algorithm
        /// @param rows
        /// @param cols
        /// @param levels
        /// @param rng
        /// @return
        std::string_view create_stringify(unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept;

        grid_manager *grid_mapper;
        processed_text_manager *processed_text_mapper;
    };

} // namespace mazes

#endif // STRINGIFY_CREATE_STATE
