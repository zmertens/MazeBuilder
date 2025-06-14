#ifndef ARGS_H
#define ARGS_H

#include <string>
#include <unordered_map>
#include <vector>
#include <ostream>
#include <optional>
#include <memory>

namespace mazes {

/// @file args.h
/// @class args
/// @brief Command-line argument handler with JSON support
/// @details Uses PIMPL pattern to wrap CLI11 functionality with additional JSON support
class args {
public:
    /// @brief Constructor
    args() noexcept;
    
    /// @brief Destructor
    ~args() noexcept;
    
    // Copy constructor
    args(const args& other) noexcept;
    
    // Move constructor
    args(args&& other) noexcept;
    
    // Copy assignment operator
    args& operator=(const args& other) noexcept;
    
    // Move assignment operator
    args& operator=(args&& other) noexcept;

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

    /// @brief Get the array of parsed configuration maps
    /// @return Reference to the vector of configuration maps
    const std::vector<std::unordered_map<std::string, std::string>>& get_array() const noexcept;

    /// @brief Check if the args contains an array of configurations
    /// @return True if args contains multiple configurations
    bool has_array() const noexcept;

    /// @brief Set a value
    /// @param key The key to set
    /// @param value The value to associate with the key
    void set(const std::string& key, const std::string& value) noexcept;

    /// @brief Add an option with a callback
    /// @param option_name The name of the option (e.g., "-o, --output")
    /// @param description Description of the option
    /// @return True if the option was successfully added
    bool add_option(const std::string& option_name, const std::string& description) noexcept;

    /// @brief Add a flag option (boolean)
    /// @param flag_name The name of the flag (e.g., "-v, --verbose")
    /// @param description Description of the flag
    /// @return True if the flag was successfully added
    bool add_flag(const std::string& flag_name, const std::string& description) noexcept;

    /// @brief Convert arguments to JSON string
    /// @return JSON string representation of all arguments
    static std::string to_str(const args& a) noexcept;

private:
    /// @brief Remove whitespace from a string
    /// @param str The string to trim
    /// @return Trimmed string
    std::string trim(const std::string& str) const noexcept;
    
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
    std::unordered_map<std::string, std::string> args_map;
    std::vector<std::unordered_map<std::string, std::string>> args_array;
};

}
#endif // ARGS_H
