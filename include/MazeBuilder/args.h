#ifndef ARGS_H
#define ARGS_H

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace mazes {

/// @file args.h
/// @class args
/// @brief Command-line argument handler with JSON support
/// @details Uses PIMPL pattern to wrap CLI11 functionality with additional JSON support
class args final {
public:
    /// @brief Default constructor
    /// @details Initializes the CLI11 app and sets up common options
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
    /// @return True if parsing was successful
    bool parse(const std::vector<std::string>& arguments) noexcept;

    /// @brief Parse program arguments from a string
    /// @param arguments Space-delimited command-line arguments
    /// @return True if parsing was successful
    bool parse(const std::string& arguments) noexcept;
    
    /// @brief Parse program arguments from argc/argv
    /// @param argc Argument count
    /// @param argv Argument values
    /// @return True if parsing was successful
    bool parse(int argc, char** argv) noexcept;

    /// @brief Clear the arguments map
    void clear() noexcept;

    /// @brief Get a value from the args map
    /// @param key The key to look up 
    /// @return The value if found, std::nullopt otherwise
    std::optional<std::string> get(const std::string& key) const noexcept;

    /// @brief Get entire args map
    /// @return Reference to the internal arguments map
    const std::unordered_map<std::string, std::string>& get() const noexcept;

    /// @brief Add a new option to the CLI parser
    /// @param flags Command line flags (e.g., "-x,--extra")
    /// @param description Help description for the option
    /// @return True if the option was successfully added
    bool add_option(const std::string& flags, const std::string& description) noexcept;

    /// @brief Add a new flag to the CLI parser
    /// @param flags Command line flags (e.g., "-f,--flag")
    /// @param description Help description for the flag
    /// @return True if the flag was successfully added
    bool add_flag(const std::string& flags, const std::string& description) noexcept;

private:
    // String constants for command-line arguments
    // Row/column related constants
    static constexpr const char* ROW_FLAG_STR = "-r";
    static constexpr const char* ROW_OPTION_STR = "--rows";
    static constexpr const char* ROW_WORD_STR = "rows";
    static constexpr const char* ROW_SHORT_STR = "r";
    
    static constexpr const char* COLUMN_FLAG_STR = "-c";
    static constexpr const char* COLUMN_OPTION_STR = "--columns";
    static constexpr const char* COLUMN_WORD_STR = "columns";
    static constexpr const char* COLUMN_SHORT_STR = "c";
    
    // JSON related constants
    static constexpr const char* JSON_FLAG_STR = "-j";
    static constexpr const char* JSON_OPTION_STR = "--json";
    static constexpr const char* JSON_WORD_STR = "json";
    static constexpr const char* JSON_SHORT_STR = "j";
    
    // Output related constants
    static constexpr const char* OUTPUT_FLAG_STR = "-o";
    static constexpr const char* OUTPUT_OPTION_STR = "--output";
    static constexpr const char* OUTPUT_WORD_STR = "output";
    static constexpr const char* OUTPUT_SHORT_STR = "o";
    static constexpr const char* DEFAULT_OUTPUT_FILENAME = "output.json";
    
    // Seed related constants
    static constexpr const char* SEED_FLAG_STR = "-s";
    static constexpr const char* SEED_OPTION_STR = "--seed";
    static constexpr const char* SEED_WORD_STR = "seed";
    static constexpr const char* SEED_SHORT_STR = "s";
    
    // Distances related constants
    static constexpr const char* DISTANCES_FLAG_STR = "-d";
    static constexpr const char* DISTANCES_OPTION_STR = "--distances";
    static constexpr const char* DISTANCES_WORD_STR = "distances";
    static constexpr const char* DISTANCES_SHORT_STR = "d";
    static constexpr const char* DISTANCES_START_STR = "distances_start";
    static constexpr const char* DISTANCES_END_STR = "distances_end";
    static constexpr const char* DISTANCES_DEFAULT_START = "0";
    static constexpr const char* DISTANCES_DEFAULT_END = "-1";
    
    // Help related constants
    static constexpr const char* HELP_FLAG_STR = "-h";
    static constexpr const char* HELP_OPTION_STR = "--help";
    static constexpr const char* HELP_WORD_STR = "help";
    static constexpr const char* HELP_SHORT_STR = "h";
    
    // Version related constants
    static constexpr const char* VERSION_FLAG_STR = "-v";
    static constexpr const char* VERSION_OPTION_STR = "--version";
    static constexpr const char* VERSION_WORD_STR = "version";
    static constexpr const char* VERSION_SHORT_STR = "v";
    
    // Algorithm related constants
    static constexpr const char* ALGO_OPTION_STR = "--algo";
    static constexpr const char* ALGO_WORD_STR = "algo";
    
    // Special values
    static constexpr const char* TRUE_VALUE = "true";

    /// @brief Parse a sliced array string into individual elements
    /// @param value The sliced array string (e.g., "[1:10]") with inclusive bounds
    /// @param args_map The map to populate with parsed values
    /// @return True if parsing was successful
    bool parse_sliced_array(const std::string& value, std::unordered_map<std::string, std::string>& args_map) noexcept;

    /// @brief Generates an output filename based on input filename or default
    /// @param input_value The input filename or JSON string
    /// @param is_string_input Whether the input is a JSON string (true) or filename (false)
    /// @return The generated output filename
    std::string generate_output_filename(const std::string& input_value, bool is_string_input) const noexcept;
    
    /// @brief Process JSON input from file or string
    /// @param json_input The JSON input
    /// @param is_string_input Whether it's a string or file
    /// @param key The command-line key used
    /// @return True if JSON was successfully processed
    bool process_json_input(const std::string& json_input, bool is_string_input, const std::string& key) noexcept;
    
    // Private implementation class (PIMPL idiom)
    class impl;
    std::unique_ptr<impl> pimpl;
};

}
#endif // ARGS_H
