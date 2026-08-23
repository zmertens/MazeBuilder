#ifndef ARGS_H
#define ARGS_H

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

/// @namespace mazes
/// @file args.h
namespace mazes
{
    /// @class args
    /// @brief Command-line argument handler with JSON support
    /// @details
    class args final
    {
    public:
        static constexpr auto APP_KEY = "app";

        static constexpr auto ALGO_ID_FLAG_STR = "-a";
        static constexpr auto ALGO_ID_OPTION_STR = "--algo";
        static constexpr auto ALGO_ID_WORD_STR = "algo";

        static constexpr auto ROW_FLAG_STR = "-r";
        static constexpr auto ROW_OPTION_STR = "--rows";
        static constexpr auto ROW_WORD_STR = "rows";

        static constexpr auto COLUMN_FLAG_STR = "-c";
        static constexpr auto COLUMN_OPTION_STR = "--columns";
        static constexpr auto COLUMN_WORD_STR = "columns";

        static constexpr auto LEVEL_FLAG_STR = "-l";
        static constexpr auto LEVEL_OPTION_STR = "--levels";
        static constexpr auto LEVEL_WORD_STR = "levels";

        // JSON related constants
        static constexpr auto JSON_FLAG_STR = "-j";
        static constexpr auto JSON_OPTION_STR = "--json";
        static constexpr auto JSON_WORD_STR = "json";

        // Output related constants
        static constexpr auto OUTPUT_ID_FLAG_STR = "-o";
        static constexpr auto OUTPUT_ID_OPTION_STR = "--output";
        static constexpr auto OUTPUT_ID_WORD_STR = "output";

        // Seed related constants
        static constexpr auto SEED_FLAG_STR = "-s";
        static constexpr auto SEED_OPTION_STR = "--seed";
        static constexpr auto SEED_WORD_STR = "seed";

        // Distances related constants
        static constexpr auto DISTANCES_FLAG_STR = "-d";
        static constexpr auto DISTANCES_OPTION_STR = "--distances";
        static constexpr auto DISTANCES_WORD_STR = "distances";
        static constexpr auto DISTANCES_START_VAL_STR = "distances_start";
        static constexpr auto DISTANCES_END_VAL_STR = "distances_end";

        // Mask related constants
        static constexpr auto MASK_FLAG_STR = "-m";
        static constexpr auto MASK_OPTION_STR = "--mask";
        static constexpr auto MASK_WORD_STR = "mask";

        // Special values
        static constexpr auto TRUE_VALUE = "true";

        /// @brief Default constructor
        /// @details Initializes the implementation pointer
        args() noexcept;

        /// @brief Destructor
        /// @details Cleans up the internal implementation pointer
        ~args();

        /// @brief Copy constructor
        /// @param other The other args object to copy from
        args(const args& other);

        /// @brief Copy assignment operator
        /// @param other The other args object to copy from
        /// @return Reference to this object
        args& operator=(const args& other);

        /// @brief Move constructor
        /// @param other The other args object to move from
        args(args&& other) noexcept;

        /// @brief Move assignment operator
        /// @param other The other args object to move from
        /// @return Reference to this object
        args& operator=(args&& other) noexcept;

        /// @brief Parse program arguments from a vector of strings
        /// @param arguments Command-line arguments
        /// @param has_program_name_as_first_arg Whether the first argument is the program name
        /// @return True if parsing was successful
        bool parse(const std::vector<std::string>& arguments,
            bool has_program_name_as_first_arg = false) const noexcept;

        /// @brief Parse program arguments from a string
        /// @param arguments Space-delimited command-line arguments
        /// @param has_program_name_as_first_arg Whether the first argument is the program name
        /// @return True if parsing was successful
        bool parse(const std::string& arguments, bool has_program_name_as_first_arg = false) const noexcept;

        /// @brief Parse program arguments from argc/argv
        /// @param argc Argument count
        /// @param argv Argument values
        /// @param has_program_name_as_first_arg Whether the first argument is the program name
        /// @return True if parsing was successful
        bool parse(int argc, char** argv, bool has_program_name_as_first_arg = false) const noexcept;

        /// @brief Clear the arguments map
        void clear() const noexcept;

        /// @brief Get a value
        /// @param key The key to look up
        /// @return The value if found, std::nullopt otherwise
        [[nodiscard]] std::optional<std::string> get(const std::string& key) const noexcept;

        /// @brief Get args from front
        /// @return The internal arguments map or empty map if not valid
        [[nodiscard]] std::unordered_map<std::string, std::string> front() const noexcept;

        /// @brief Remove the first parsed argument map
        /// @return True if an argument map was removed
        bool pop_front() const noexcept;

        /// @brief Get entire args map
        /// @return The internal arguments map or empty map if not valid
        [[nodiscard]] std::vector<std::unordered_map<std::string, std::string>> get() const noexcept;

        /// @brief Get the number of argument maps
        /// @return The number of argument maps
        [[nodiscard]] std::size_t count() const noexcept;
    private:
        class impl;
        std::unique_ptr<impl> pimpl;
    };
}
#endif // ARGS_H
