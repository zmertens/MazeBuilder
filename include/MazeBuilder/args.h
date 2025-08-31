#ifndef ARGS_H
#define ARGS_H

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mazes {

/// @file args.h
/// @class args
/// @brief Command-line argument handler with JSON support
/// @details Uses PIMPL pattern to wrap CLI11 functionality with additional JSON support
class args final {
public:

    static constexpr const auto APP_KEY = "app";

    static constexpr const auto ALGO_ID_FLAG_STR = "-a";
    static constexpr const auto ALGO_ID_OPTION_STR = "--algo";
    static constexpr const auto ALGO_ID_WORD_STR = "algo";

    static constexpr const auto BLOCK_ID_FLAG_STR = "-b";
    static constexpr const auto BLOCK_ID_OPTION_STR = "--block";
    static constexpr const auto BLOCK_ID_WORD_STR = "block";

    static constexpr const auto ROW_FLAG_STR = "-r";
    static constexpr const auto ROW_OPTION_STR = "--rows";
    static constexpr const auto ROW_WORD_STR = "rows";
    
    static constexpr const auto COLUMN_FLAG_STR = "-c";
    static constexpr const auto COLUMN_OPTION_STR = "--columns";
    static constexpr const auto COLUMN_WORD_STR = "columns";

    static constexpr const auto LEVEL_FLAG_STR = "-l";
    static constexpr const auto LEVEL_OPTION_STR = "--levels";
    static constexpr const auto LEVEL_WORD_STR = "levels";
    
    // JSON related constants
    static constexpr const auto JSON_FLAG_STR = "-j";
    static constexpr const auto JSON_OPTION_STR = "--json";
    static constexpr const auto JSON_WORD_STR = "json";
    
    // Output related constants
    static constexpr const auto OUTPUT_ID_FLAG_STR = "-o";
    static constexpr const auto OUTPUT_ID_OPTION_STR = "--output";
    static constexpr const auto OUTPUT_ID_WORD_STR = "output";
    static constexpr const auto DEFAULT_OUTPUT_FILENAME = "maze.txt";
    
    // Output filename related constants
    static constexpr const auto OUTPUT_FILENAME_WORD_STR = "output_filename";
    
    // Seed related constants
    static constexpr const auto SEED_FLAG_STR = "-s";
    static constexpr const auto SEED_OPTION_STR = "--seed";
    static constexpr const auto SEED_WORD_STR = "seed";
    
    // Distances related constants
    static constexpr const auto DISTANCES_FLAG_STR = "-d";
    static constexpr const auto DISTANCES_OPTION_STR = "--distances";
    static constexpr const auto DISTANCES_WORD_STR = "distances";
    static constexpr const auto DISTANCES_START_STR = "distances_start";
    static constexpr const auto DISTANCES_END_STR = "distances_end";
    
    // Help related constants
    static constexpr const auto HELP_FLAG_STR = "-h";
    static constexpr const auto HELP_OPTION_STR = "--help";
    static constexpr const auto HELP_WORD_STR = "help";
    
    // Version related constants
    static constexpr const auto VERSION_FLAG_STR = "-v";
    static constexpr const auto VERSION_OPTION_STR = "--version";
    static constexpr const auto VERSION_WORD_STR = "version";
    
    // Special values
    static constexpr const auto TRUE_VALUE = "true";
    static constexpr const auto FALSE_VALUE = "false";

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
    args(args&& other) noexcept = default;

    /// @brief Move assignment operator
    /// @param other The other args object to move from
    /// @return Reference to this object
    args& operator=(args&& other) noexcept = default;

    /// @brief Parse program arguments from a vector of strings
    /// @param arguments Command-line arguments
    /// @param has_program_name_as_first_arg Whether the first argument is the program name
    /// @return True if parsing was successful
    bool parse(const std::vector<std::string>& arguments, bool has_program_name_as_first_arg = false) noexcept;

    /// @brief Parse program arguments from a string
    /// @param arguments Space-delimited command-line arguments
    /// @param has_program_name_as_first_arg Whether the first argument is the program name
    /// @return True if parsing was successful
    bool parse(const std::string& arguments, bool has_program_name_as_first_arg = false) noexcept;
    
    /// @brief Parse program arguments from argc/argv
    /// @param argc Argument count
    /// @param argv Argument values
    /// @param has_program_name_as_first_arg Whether the first argument is the program name
    /// @return True if parsing was successful
    bool parse(int argc, char** argv, bool has_program_name_as_first_arg = false) noexcept;

    /// @brief Clear the arguments map
    void clear() noexcept;

    /// @brief Get a value from the args map (from front)
    /// @param key The key to look up 
    /// @return The value if found, std::nullopt otherwise
    std::optional<std::string> get(const std::string& key) const noexcept;

    /// @brief Get entire args map (from front)
    /// @return The internal arguments map or empty map if not valid
    std::optional<std::unordered_map<std::string, std::string>> get() const noexcept;

    /// @brief Get vector of args maps (useful for JSON parsing with array of objects)
    /// @return The internal arguments map vector or empty vector if not valid
    std::optional<std::vector<std::unordered_map<std::string, std::string>>> get_array() const noexcept;

private:

    /// @brief Process JSON input from file or string
    /// @param json_input The JSON input
    /// @param is_string_input Whether it's a string or file
    /// @param key The command-line key used
    /// @return True if JSON was successfully processed
    bool process_json_input(std::string_view json_input) noexcept;
    
    // Private implementation class (PIMPL idiom)
    class impl;
    std::unique_ptr<impl> pimpl;
};

}
#endif // ARGS_H
