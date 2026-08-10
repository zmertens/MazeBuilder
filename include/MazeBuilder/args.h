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

        static constexpr auto BLOCK_ID_FLAG_STR = "-b";
        static constexpr auto BLOCK_ID_OPTION_STR = "--block";
        static constexpr auto BLOCK_ID_WORD_STR = "block";

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
        static constexpr auto DEFAULT_OUTPUT_FILENAME = "maze.txt";

        // Output filename related constants
        static constexpr auto OUTPUT_FILENAME_WORD_STR = "output_filename";

        // Seed related constants
        static constexpr auto SEED_FLAG_STR = "-s";
        static constexpr auto SEED_OPTION_STR = "--seed";
        static constexpr auto SEED_WORD_STR = "seed";

        // Distances related constants
        static constexpr auto DISTANCES_FLAG_STR = "-d";
        static constexpr auto DISTANCES_OPTION_STR = "--distances";
        static constexpr auto DISTANCES_WORD_STR = "distances";
        static constexpr auto DISTANCES_START_STR = "distances_start";
        static constexpr auto DISTANCES_END_STR = "distances_end";

        // Image dimension related constants
        static constexpr auto IMAGE_WIDTH_WORD_STR = "image_width";
        static constexpr auto IMAGE_HEIGHT_WORD_STR = "image_height";
        static constexpr auto IMAGE_WIDTH_OPTION_STR = "--image-width";
        static constexpr auto IMAGE_HEIGHT_OPTION_STR = "--image-height";
        static constexpr auto IMAGE_WIDTH_FLAG_STR = "-W";
        static constexpr auto IMAGE_HEIGHT_FLAG_STR = "-H";

        // Help related constants
        static constexpr auto HELP_FLAG_STR = "-h";
        static constexpr auto HELP_OPTION_STR = "--help";
        static constexpr auto HELP_WORD_STR = "help";

        // Version related constants
        static constexpr auto VERSION_FLAG_STR = "-v";
        static constexpr auto VERSION_OPTION_STR = "--version";
        static constexpr auto VERSION_WORD_STR = "version";

        // Mask related constants
        static constexpr auto MASK_FLAG_STR = "-m";
        static constexpr auto MASK_OPTION_STR = "--mask";
        static constexpr auto MASK_WORD_STR = "mask";

        // Step visualization constants
        static constexpr auto SHOW_STEPS_FLAG_STR = "-S";
        static constexpr auto SHOW_STEPS_OPTION_STR = "--show-steps";
        static constexpr auto SHOW_STEPS_WORD_STR = "show_steps";

        // Special values
        static constexpr auto TRUE_VALUE = "true";
        static constexpr auto FALSE_VALUE = "false";

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

        /// @brief Get entire args map (from front)
        /// @return The internal arguments map or empty map if not valid
        [[nodiscard]] std::optional<std::unordered_map<std::string, std::string>> get() const noexcept;

        /// @brief Get vector of args maps (useful for JSON parsing with array of objects)
        /// @return The internal arguments map vector or empty vector if not valid
        [[nodiscard]] std::optional<std::vector<std::unordered_map<std::string, std::string>>>
        get_array() const noexcept;

    private:
        // Private implementation class (PIMPL idiom)
        class impl;
        std::unique_ptr<impl> pimpl;
    };
}
#endif // ARGS_H
