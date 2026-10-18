#ifndef STATE_UTILS_H
#define STATE_UTILS_H

#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/state.h>

#include <string>
#include <unordered_map>

/// @brief Namespace containing utility functions for maze builder states
/// @file state_utils.h
namespace mazes
{
    class args;
    class grid_operations;
    class randomizer;
    class runtime_app;
    class context;

    class state_utils
    {
    public:
        /// @brief Parses the maze dimensions from the given arguments.
        /// @param args The arguments containing the dimension information.
        /// @param rows Reference to store the number of rows.
        /// @param cols Reference to store the number of columns.
        /// @param levels Reference to store the number of levels.
        static void parse_dimensions(const std::unordered_map<std::string, std::string> &args, unsigned int &rows, unsigned int &cols,
                                     unsigned int &levels) noexcept;

        /// @brief Checks if the "distances" argument is present in the given arguments.
        /// @details This function checks if the "distances" argument is present in the provided
        /// @param args The arguments to check for the "distances" key.
        /// @return True if the "distances" argument is present, false otherwise.
        static bool has_distances(const std::unordered_map<std::string, std::string> &args) noexcept;

        /// @brief Builds a configurator from the front entry of the given arguments.
        /// @details Shared by every state implementing create_contract so arg parsing lives in one place.
        /// @param a The parsed arguments
        /// @param default_algo The algorithm used when the arguments do not select one
        /// @return A configurator populated with dimensions, distances, algorithm and mask source
        static configurator build_configurator(const args &a, const algo default_algo = algo::DFS) noexcept;

        /// @brief Determines the output state based on the given arguments.
        /// @param args The arguments containing the output information.
        /// @return The corresponding state::ID for the output.
        static state::ID output_state_for(const std::unordered_map<std::string, std::string> &args) noexcept;

        static bool advance_args(const runtime_app::context &ctx) noexcept;

        // Reseeds rng from the parsed "seed" arg so repeated apply() calls with the same
        // seed reproduce identical topology; the rng is otherwise shared and keeps advancing.
        static void reseed_from_args(const std::unordered_map<std::string, std::string> &args, randomizer &rng) noexcept;

        template <typename... Mappers>
        static void validate_mappers(Mappers &&...mapper) noexcept;

        /// @brief Resets the grid's topology (links) before each generation.
        /// @details resize() always clears cell links even when dimensions are unchanged;
        /// without this, repeated maze generation at the same size (e.g. rebuilding on
        /// keypress) would keep linking new random walls on top of the previous maze's
        /// links, so the maze gradually loses walls / becomes fully open over time.
        /// @param ops Pointer to the grid operations.
        /// @param rows Desired number of rows.
        /// @param cols Desired number of columns.
        /// @param levels Desired number of levels.
        static void check_before_resize(grid_operations *ops, unsigned int rows, unsigned int cols, unsigned int levels) noexcept;

    }; // class state_utils
} // namespace mazes

template <typename... Mappers>
void mazes::state_utils::validate_mappers(Mappers &&...mapper) noexcept
{
    if (!((mapper != nullptr) && ...))
    {
        async_logger::instance()->log("mappers are null");
    }
}

#endif // STATE_UTILS_H
