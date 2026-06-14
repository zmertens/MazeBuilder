#ifndef OBJECTIFY_CREATE_STATE
#define OBJECTIFY_CREATE_STATE

#include <MazeBuilder/algos.h>
#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/state.h>

#include <optional>
#include <string_view>

/// @file objectify_create_state.h
/// @namespace mazes
namespace mazes
{
    class args;
    class randomizer;
    class runtime_stack;
    struct context;

    /// @brief State for creating a maze using the objectify algorithm
    class objectify_create_state final : public create_contract, public state
    {
    public:
        explicit objectify_create_state(const runtime_app::context &ctx, runtime_stack *stack);

        /// @brief Creates a maze using the objectify algorithm
        /// @param a
        /// @param rows
        /// @param cols
        /// @param levels
        /// @param rng
        /// @return
        virtual std::string_view create(algo a, unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept override;

        void draw() const noexcept override;

        /// @brief Updates the state of the objectify creation process
        /// @param args Optional arguments for the update
        /// @param delta_time Time elapsed since the last update
        /// @return True if the update was successful, false otherwise
        bool update(const std::optional<args> &args, double delta_time) noexcept override;

    private:
        /// @brief Implementation of the objectify maze creation algorithm
        /// @param rows
        /// @param cols
        /// @param levels
        /// @param rng
        /// @return
        std::string_view objectify(unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept;

        grid_manager *grid_mapper;
        processed_text_manager *processed_text_mapper;
    };

} // namespace mazes

#endif // OBJECTIFY_CREATE_STATE
