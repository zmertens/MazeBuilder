#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/string_view_utils.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <CLI11/CLI11.hpp>

using namespace mazes;

// Implementation class using CLI11 best practices
class args::impl {

public:
    impl() : cli_app(DEFAULT_CLI_IMPLEMENTATION_NAME) {
        setup_cli();
    }
    
    // Our internal storage maps for compatibility
    std::unordered_map<std::string, std::string> args_map;
    
    // Storage for JSON array processing
    std::vector<std::unordered_map<std::string, std::string>> args_map_vec;

    // Direct variable bindings for CLI11
    std::vector<std::string> json_inputs;
    std::vector<std::string> output_files;
    std::vector<int> rows_values;
    std::vector<int> columns_values;
    std::vector<int> levels_values;
    std::vector<int> seed_values;
    std::vector<std::string> algo_values;
    std::vector<std::string> distances_values;

    // Flag tracking
    bool help_flag = false;
    bool version_flag = false;
    bool distances_flag = false;

    // CLI11 app
    CLI::App cli_app;

private:
    static constexpr auto DEFAULT_CLI_IMPLEMENTATION_NAME = "CLI11_MB";

public:
    // Helper method to add argument variants for storing flags, options, and words
    void add_argument_variants(std::string_view key, std::string_view value) {
        
        if (key == args::ROW_WORD_STR) {
            args_map[args::ROW_FLAG_STR] = value;
            args_map[args::ROW_OPTION_STR] = value;
            args_map[args::ROW_WORD_STR] = value;
        } else if (key == args::COLUMN_WORD_STR) {
            args_map[args::COLUMN_FLAG_STR] = value;
            args_map[args::COLUMN_OPTION_STR] = value;
            args_map[args::COLUMN_WORD_STR] = value;
        } else if (key == args::LEVEL_WORD_STR) {
            args_map[args::LEVEL_FLAG_STR] = value;
            args_map[args::LEVEL_OPTION_STR] = value;
            args_map[args::LEVEL_WORD_STR] = value;
        } else if (key == args::SEED_WORD_STR) {
            args_map[args::SEED_FLAG_STR] = value;
            args_map[args::SEED_OPTION_STR] = value;
            args_map[args::SEED_WORD_STR] = value;
        } else if (key == args::OUTPUT_ID_WORD_STR) {
            args_map[args::OUTPUT_ID_FLAG_STR] = value;
            args_map[args::OUTPUT_ID_OPTION_STR] = value;
            args_map[args::OUTPUT_ID_WORD_STR] = value;
        } else if (key == args::JSON_WORD_STR) {
            args_map[args::JSON_FLAG_STR] = value;
            args_map[args::JSON_OPTION_STR] = value;
            args_map[args::JSON_WORD_STR] = value;
        } else if (key == args::DISTANCES_WORD_STR) {
            args_map[args::DISTANCES_FLAG_STR] = value;
            args_map[args::DISTANCES_OPTION_STR] = value;
            args_map[args::DISTANCES_WORD_STR] = value;
        } else if (key == args::ALGO_ID_WORD_STR) {
            args_map[args::ALGO_ID_FLAG_STR] = value;
            args_map[args::ALGO_ID_OPTION_STR] = value;
            args_map[args::ALGO_ID_WORD_STR] = value;
        } else if (key == args::HELP_WORD_STR) {
            args_map[args::HELP_FLAG_STR] = value;
            args_map[args::HELP_OPTION_STR] = value;
            args_map[args::HELP_WORD_STR] = value;
        } else if (key == args::VERSION_WORD_STR) {
            args_map[args::VERSION_FLAG_STR] = value;
            args_map[args::VERSION_OPTION_STR] = value;
            args_map[args::VERSION_WORD_STR] = value;
        } else {
            // For other keys, store as-is (like app name)
            if (!key.empty() && !(key[0] == '.' || key[0] == '/' || key[0] == '-')) {
                args_map[std::string{ key }] = value;
            }
        }
    }

    // Validate slice syntax before processing
    bool validate_slice_syntax(const std::string& input) {
        // Check if it contains slice-like patterns but malformed
        if (input.find(':') != std::string::npos) {
            // Should have proper bracket structure [start:end]
            std::regex valid_slice_pattern(R"(\[.*?:.*?\])");
            if (!std::regex_search(input, valid_slice_pattern)) {
                return false; // Contains colon but not valid slice syntax
            }
            
            // Additional validation for malformed brackets
            if (input.find(']') != std::string::npos && input.find('[') == std::string::npos) {
                return false; // Has closing bracket but no opening
            }
            if (input.find('[') != std::string::npos && input.find(']') == std::string::npos) {
                return false; // Has opening bracket but no closing
            }
            
            // Check for wrong bracket order
            auto open_pos = input.find('[');
            auto close_pos = input.find(']');
            if (open_pos != std::string::npos && close_pos != std::string::npos && open_pos > close_pos) {
                return false; // Closing bracket before opening bracket
            }
        }
        
        // Check for malformed brackets without colons
        if ((input.find('[') != std::string::npos || input.find(']') != std::string::npos) && 
            input.find(':') == std::string::npos) {
            return false; // Has brackets but no colon
        }
        
        return true;
    }

    // Pre-validate arguments before passing to CLI11
    bool pre_validate_arguments(const std::vector<std::string>& args) {

        for (auto i{ 0 }; i < args.size(); ++i) {

            const auto& arg = args[i];
            
            // Check for malformed distances arguments
            if (arg == args::DISTANCES_FLAG_STR || arg == args::DISTANCES_OPTION_STR) {

                // Check if next argument is a malformed slice
                if (i + 1 < args.size()) {

                    const auto& next_arg = args[i + 1];

                    if (!validate_slice_syntax(next_arg)) {

                        return false;
                    }
                }
            }
            
            // Check for arguments with embedded slice syntax
            if (arg.find(args::DISTANCES_FLAG_STR) == 0 || arg.find(args::DISTANCES_OPTION_STR) == 0) {

                if (!validate_slice_syntax(arg)) {

                    return false;
                }
            }
            
            // Check for unknown options
            if (arg.length() > 1 && arg[0] == '-') {
                // Skip if it's a known option
                if (arg == args::ROW_FLAG_STR || arg == args::ROW_OPTION_STR ||
                    arg == args::COLUMN_FLAG_STR || arg == args::COLUMN_OPTION_STR ||
                    arg == args::LEVEL_FLAG_STR || arg == args::LEVEL_OPTION_STR ||
                    arg == args::SEED_FLAG_STR || arg == args::SEED_OPTION_STR ||
                    arg == args::ALGO_ID_FLAG_STR || arg == args::ALGO_ID_OPTION_STR ||
                    arg == args::OUTPUT_ID_FLAG_STR || arg == args::OUTPUT_ID_OPTION_STR ||
                    arg == args::JSON_FLAG_STR || arg == args::JSON_OPTION_STR ||
                    arg == args::DISTANCES_FLAG_STR || arg == args::DISTANCES_OPTION_STR ||
                    arg == args::HELP_FLAG_STR || arg == args::HELP_OPTION_STR ||
                    arg == args::VERSION_FLAG_STR || arg == args::VERSION_OPTION_STR) {

                    continue;
                }
                
                // Check for option=value format
                auto eq_pos = arg.find('=');
                if (eq_pos != std::string::npos) {
                    std::string option_part = arg.substr(0, eq_pos);
                    if (option_part == args::ROW_OPTION_STR || option_part == args::COLUMN_OPTION_STR ||
                        option_part == args::LEVEL_OPTION_STR || option_part == args::SEED_OPTION_STR ||
                        option_part == args::ALGO_ID_OPTION_STR || option_part == args::OUTPUT_ID_OPTION_STR ||
                        option_part == args::JSON_OPTION_STR || option_part == args::DISTANCES_OPTION_STR ||
                        option_part == args::HELP_OPTION_STR || option_part == args::VERSION_OPTION_STR) {
                        // Validate the value part for slice syntax if it's distances
                        if (option_part == args::DISTANCES_OPTION_STR) {
                            std::string value_part = arg.substr(eq_pos + 1);
                            if (!validate_slice_syntax(value_part)) {
                                return false;
                            }
                        }
                        continue;
                    }
                }
                
                // Check for concatenated short options (like -r10, -adfs)
                if (arg.length() > 2 && arg[1] != '-') {

                    char short_opt = arg[1];

                    if (short_opt == 'r' || short_opt == 'c' || short_opt == 'l' || 
                        short_opt == 's' || short_opt == 'a' || short_opt == 'o' || 
                        short_opt == 'j' || short_opt == 'd' || short_opt == 'h' || short_opt == 'v') {
                        continue;
                    }
                }
                
                // If we reach here, it's an unknown option
                return false;
            }
            
            // Check for standalone positional arguments that aren't app names or values
            if (arg[0] != '-') {
                // Allow if previous argument was an option that expects a value
                if (i > 0) {
                    const auto& prev_arg = args[i - 1];
                    if (prev_arg == args::ROW_FLAG_STR || prev_arg == args::COLUMN_FLAG_STR ||
                        prev_arg == args::LEVEL_FLAG_STR || prev_arg == args::SEED_FLAG_STR ||
                        prev_arg == args::ALGO_ID_FLAG_STR || prev_arg == args::OUTPUT_ID_FLAG_STR ||
                        prev_arg == args::JSON_FLAG_STR || prev_arg == args::DISTANCES_FLAG_STR) {
                        continue;
                    }
                }
                
                // If this is the first argument and we have no options before it,
                // it could be a program name that wasn't filtered out, so reject it
                return false;
            }
        }
        
        return true;
    }

    void setup_cli() {

        using namespace std;

        // Configure CLI11 app to be more strict
        cli_app.allow_extras(false);  // Don't allow unknown arguments
        cli_app.prefix_command(false);
        cli_app.ignore_case(false); // Be case sensitive for better error detection
        
        // Disable automatic help so we can handle it ourselves
        cli_app.set_help_flag();
        cli_app.set_version_flag();
        
        // Add options with direct vector binding
        static const string JSON_OPTIONS = args::JSON_FLAG_STR + string(",") + args::JSON_OPTION_STR;
        cli_app.add_option(JSON_OPTIONS, json_inputs, "Parse JSON input file or string")
            ->capture_default_str();

        static const string OUTPUT_OPTIONS = args::OUTPUT_ID_FLAG_STR + string(",") + args::OUTPUT_ID_OPTION_STR;
        cli_app.add_option(OUTPUT_OPTIONS, output_files, "Output file")
            ->capture_default_str();

        static const string ROWS_OPTIONS = args::ROW_FLAG_STR + string(",") + args::ROW_OPTION_STR;
        cli_app.add_option(ROWS_OPTIONS, rows_values, "Number of rows in the maze")
            ->capture_default_str();

        static const string COLUMNS_OPTIONS = args::COLUMN_FLAG_STR + string(",") + args::COLUMN_OPTION_STR;
        cli_app.add_option(COLUMNS_OPTIONS, columns_values, "Number of columns in the maze")
            ->capture_default_str();

        static const string LEVELS_OPTIONS = args::LEVEL_FLAG_STR + string(",") + args::LEVEL_OPTION_STR;
        cli_app.add_option(LEVELS_OPTIONS, levels_values, "Number of levels in the maze")
            ->capture_default_str();

        static const string SEED_OPTIONS = args::SEED_FLAG_STR + string(",") + args::SEED_OPTION_STR;
        cli_app.add_option(SEED_OPTIONS, seed_values, "Random seed for maze generation")
            ->capture_default_str();

        static const string ALGO_OPTIONS = args::ALGO_ID_FLAG_STR + string(",") + args::ALGO_ID_OPTION_STR;
        cli_app.add_option(ALGO_OPTIONS, algo_values, "Algorithm to use for maze generation")
            ->capture_default_str();
            
        // Special handling for distances which can be flag or option with sliced array syntax
        static const string DISTANCE_OPTIONS = args::DISTANCES_FLAG_STR + string(",") + args::DISTANCES_OPTION_STR;
        cli_app.add_option(DISTANCE_OPTIONS, distances_values,
            "Calculate distances between cells, optionally with a range [start:end]")
            ->expected(0, 1)
            ->capture_default_str();
            
        // Add flags manually to avoid automatic exit behavior
        static const string HELP_OPTIONS = args::HELP_FLAG_STR + string(",") + args::HELP_OPTION_STR;
        cli_app.add_flag(HELP_OPTIONS, help_flag, "Show help information");

        static const string VERSION_OPTIONS = args::VERSION_FLAG_STR + string(",") + args::VERSION_OPTION_STR;
        cli_app.add_flag(VERSION_OPTIONS, version_flag, "Show version information");
    }

    bool parse(int argc, char** argv, bool has_program_name_at_first_index = true) {

        // Convert to vector for pre-validation
        std::vector<std::string> args_vector;
        for (int i = has_program_name_at_first_index ? 1 : 0; i < argc; ++i) {

            if (argv[i]) {

                args_vector.emplace_back(argv[i]);
            }
        }
        
        // Pre-validate arguments
        if (!pre_validate_arguments(args_vector)) {

            return false;
        }
        
        try {
            cli_app.parse(argc, argv);

            populate_args_map();

        } catch (const CLI::ParseError& e) {
            // Handle help and version requests gracefully
            if (e.get_exit_code() == 0) {
                populate_args_map();
                return true;
            }

            return false;
        } catch (const std::exception& e) {

            // Catch any other exceptions and return false
            std::cerr << "Error parsing arguments: " << e.what() << std::endl;

            return false;
        }
        
        return true;
    }

    // Convert vectors to our internal args_map format for backward compatibility
    void populate_args_map() {
        args_map.clear();
        
        // Store any extra arguments (like app names) as positional arguments
        if (auto extras{ cli_app.remaining() }; !extras.empty()) {
            // Store the first extra as the app name
            add_argument_variants("app", extras[0]);
            // Store any other extras
            for (size_t i = 1; i < extras.size(); ++i) {
                args_map["extra_" + std::to_string(i)] = extras[i];
            }
        }
        
        // Handle JSON inputs and process them
        if (!json_inputs.empty()) {
            std::string value = json_inputs.back();
            add_argument_variants(args::JSON_WORD_STR, value);
            
            // Strip whitespace to determine if it's a JSON string vs file
            std::string trimmed_value = std::string(string_view_utils::strip(value));
            
            // Process JSON if it's a string (starts with ` after trimming)
            if (!trimmed_value.empty() && trimmed_value.front() == '`') {
                if (!process_json_string(value)) {

                    throw std::runtime_error("Invalid JSON input: " + value);
                }
            } else {
                // Assume it's a file and process file-based JSON
                if (!process_json_file(value)) {

                    throw std::runtime_error("Failed to load JSON file: " + value);
                }
            }
        }
        
        // Handle output files
        if (!output_files.empty()) {
            std::string value = output_files.back();
            add_argument_variants(args::OUTPUT_ID_WORD_STR, value);
        }
        
        // Handle rows
        if (!rows_values.empty()) {
            std::string value = std::to_string(rows_values.back());
            add_argument_variants(args::ROW_WORD_STR, value);
        }
        
        // Handle columns
        if (!columns_values.empty()) {
            std::string value = std::to_string(columns_values.back());
            add_argument_variants(args::COLUMN_WORD_STR, value);
        }
        
        // Handle levels
        if (!levels_values.empty()) {
            std::string value = std::to_string(levels_values.back());
            add_argument_variants(args::LEVEL_WORD_STR, value);
        }
        
        // Handle seed
        if (!seed_values.empty()) {
            std::string value = std::to_string(seed_values.back());
            add_argument_variants(args::SEED_WORD_STR, value);
        }
        
        // Handle algorithm
        if (!algo_values.empty()) {
            std::string value = algo_values.back();
            add_argument_variants(args::ALGO_ID_WORD_STR, value);
        }
        
        // Handle distances (special case with sliced array parsing)
        if (!distances_values.empty()) {
            std::string value = distances_values.back();
            
            // Check if it looks like slice array syntax was used (contains colon)
            // If so, reconstruct the brackets that CLI11 strips off
            if (value.find(':') != std::string::npos && value.front() != '[') {
                value = "[" + value + "]";
            }
            
            add_argument_variants(args::DISTANCES_WORD_STR, value);
            
            // Parse sliced array syntax
            parse_sliced_array(value);
           
        } else if (distances_flag || cli_app.count(args::DISTANCES_FLAG_STR) || cli_app.count(args::DISTANCES_OPTION_STR)) {
            // Flag form without value
            add_argument_variants(args::DISTANCES_WORD_STR, args::TRUE_VALUE);
        }
        
        // Handle flags
        if (help_flag) {
            add_argument_variants(args::HELP_WORD_STR, args::TRUE_VALUE);
        }
        
        if (version_flag) {
            add_argument_variants(args::VERSION_WORD_STR, args::TRUE_VALUE);
        }
    } // populate_args_map

    // Internal JSON processing methods for use in populate_args_map
    bool process_json_string(const std::string& json_str) {

        try {
            // Remove backticks and parse JSON
            std::string clean_json = json_str;
            
            // Strip whitespace first
            clean_json = string_view_utils::strip(clean_json);
            
            // Remove backticks if present
            if (!clean_json.empty() && clean_json.front() == '`' && clean_json.back() == '`') {
                clean_json = clean_json.substr(1, clean_json.length() - 2);
            }
            
            // Strip whitespace again after removing backticks
            clean_json = string_view_utils::strip(clean_json);
            
            // Use json_helper to parse the cleaned JSON string
            json_helper jh{};
            std::unordered_map<std::string, std::string> parsed_json;

            if (!jh.from(clean_json, parsed_json)) {

                return false;
            }

            for (const auto& [key, value] : parsed_json) {
                // Map JSON keys to argument keys and add using add_argument_variants
                if (key == "rows") {
                    add_argument_variants(args::ROW_WORD_STR, value);
                } else if (key == "columns") {
                    add_argument_variants(args::COLUMN_WORD_STR, value);
                } else if (key == "levels") {
                    add_argument_variants(args::LEVEL_WORD_STR, value);
                } else if (key == "seed") {
                    add_argument_variants(args::SEED_WORD_STR, value);
                } else if (key == "algo") {
                    add_argument_variants(args::ALGO_ID_WORD_STR, value);
                } else if (key == "output") {
                    add_argument_variants(args::OUTPUT_ID_WORD_STR, value);
                } else if (key == "distances") {
                    // Handle boolean distances field
                    if (value == "true" || value == "1") {
                        add_argument_variants(args::DISTANCES_WORD_STR, args::TRUE_VALUE);
                    } else if (value == "false" || value == "0") {
                        // Don't add distances if it's false
                    } else {
                        // Might be a slice notation as a string
                        add_argument_variants(args::DISTANCES_WORD_STR, value);
                        parse_sliced_array(value);
                    }
                } else {
                    // Store unknown JSON keys as-is
                    args_map[key] = value;
                }
            }
        } catch (const std::exception&) {

            return false;
        }

        return true;
    }
    
    // Internal JSON file processing for use in populate_args_map
    bool process_json_file(const std::string& filename) {
        try {
            json_helper jh{};
            
            // Check if file exists from current directory or tests directory
            std::string test_file_path = filename;
            std::filesystem::path fp{ test_file_path };
            if (!std::filesystem::exists(fp)) {
                // Try looking in tests directory
                test_file_path = "../tests/" + filename;
                fp = std::filesystem::path{ test_file_path };
                if (!std::filesystem::exists(fp)) {
                    // Try tests directory without ../
                    test_file_path = "tests/" + filename;
                    fp = std::filesystem::path{ test_file_path };
                    if (!std::filesystem::exists(fp)) {
                        return false;
                    }
                }
            }
            
            // First try to load as an array
            std::vector<std::unordered_map<std::string, std::string>> parsed_json_array;
            if (jh.load_array(test_file_path, parsed_json_array)) {
                // Successfully parsed JSON array file
                args_map_vec = parsed_json_array;
                
                // For backward compatibility, if there's only one object, also populate args_map
                if (!parsed_json_array.empty()) {
                    const auto& first_object = parsed_json_array[0];
                    for (const auto& [key, value] : first_object) {
                        // Map JSON keys to argument keys and add using add_argument_variants
                        if (key == "rows") {
                            add_argument_variants(args::ROW_WORD_STR, value);
                        } else if (key == "columns") {
                            add_argument_variants(args::COLUMN_WORD_STR, value);
                        } else if (key == "levels") {
                            add_argument_variants(args::LEVEL_WORD_STR, value);
                        } else if (key == "seed") {
                            add_argument_variants(args::SEED_WORD_STR, value);
                        } else if (key == "algo") {
                            add_argument_variants(args::ALGO_ID_WORD_STR, value);
                        } else if (key == "output") {
                            add_argument_variants(args::OUTPUT_ID_WORD_STR, value);
                        } else if (key == "distances") {
                            // Handle boolean distances field
                            if (value == "true" || value == "1") {
                                add_argument_variants(args::DISTANCES_WORD_STR, args::TRUE_VALUE);
                            } else if (value == "false" || value == "0") {
                                // Don't add distances if it's false
                            } else {
                                // Might be a slice notation as a string
                                add_argument_variants(args::DISTANCES_WORD_STR, value);
                                parse_sliced_array(value);
                            }
                        } else {
                            // Store unknown JSON keys as-is
                            args_map[key] = value;
                        }
                    }
                }
                return true;
            }
            
            // If array loading failed, try loading as a single object (backward compatibility)
            std::unordered_map<std::string, std::string> parsed_json;
            if (jh.load(test_file_path, parsed_json)) {
                // Successfully parsed JSON file, now add the values to args_map
                for (const auto& [key, value] : parsed_json) {
                    // Map JSON keys to argument keys and add using add_argument_variants
                    if (key == "rows") {
                        add_argument_variants(args::ROW_WORD_STR, value);
                    } else if (key == "columns") {
                        add_argument_variants(args::COLUMN_WORD_STR, value);
                    } else if (key == "levels") {
                        add_argument_variants(args::LEVEL_WORD_STR, value);
                    } else if (key == "seed") {
                        add_argument_variants(args::SEED_WORD_STR, value);
                    } else if (key == "algo") {
                        add_argument_variants(args::ALGO_ID_WORD_STR, value);
                    } else if (key == "output") {
                        add_argument_variants(args::OUTPUT_ID_WORD_STR, value);
                    } else if (key == "distances") {
                        // Handle boolean distances field
                        if (value == "true" || value == "1") {
                            add_argument_variants(args::DISTANCES_WORD_STR, args::TRUE_VALUE);
                        } else if (value == "false" || value == "0") {
                            // Don't add distances if it's false
                        } else {
                            // Might be a slice notation as a string
                            add_argument_variants(args::DISTANCES_WORD_STR, value);
                            parse_sliced_array(value);
                        }
                    } else {
                        // Store unknown JSON keys as-is
                        args_map[key] = value;
                    }
                }
                return true;
            }
        } catch (const std::exception&) {
            return false;
        }

        return false;
    }

    void clear() noexcept {
        args_map.clear();
        args_map_vec.clear();

        // Clear vectors in impl
        json_inputs.clear();
        output_files.clear();
        rows_values.clear();
        columns_values.clear();
        levels_values.clear();
        seed_values.clear();
        algo_values.clear();
        distances_values.clear();

        // Reset flags
        help_flag = false;
        version_flag = false;
        distances_flag = false;
    }

    // Public method for external access to slice parsing
    void parse_sliced_array(const std::string& value) {
        std::regex slice_pattern(R"(\[(.*?):(.*?)\])");
        std::smatch matches;

        if (std::regex_match(value, matches, slice_pattern)) {
            std::string start_idx = matches[1].str();
            std::string end_idx = matches[2].str();

            // Store the parsed start and end indices
            if (!start_idx.empty()) {
                args_map[args::DISTANCES_START_STR] = start_idx;
            } else {
                args_map[args::DISTANCES_START_STR] = std::to_string(configurator::DEFAULT_DISTANCES_START);
            }

            if (!end_idx.empty()) {
                args_map[args::DISTANCES_END_STR] = end_idx;
            } else {
                args_map[args::DISTANCES_END_STR] = std::to_string(configurator::DEFAULT_DISTANCES_END);
            }
        }
    }
}; // impl

args::args() noexcept : pimpl{ std::make_unique<impl>() } {
}

args::~args() = default;

args::args(const args& other) : pimpl{ std::make_unique<impl>() } {
    if (other.pimpl) {
        
        pimpl->args_map = other.pimpl->args_map;
        pimpl->args_map_vec = other.pimpl->args_map_vec;
        
        pimpl->json_inputs = other.pimpl->json_inputs;
        pimpl->output_files = other.pimpl->output_files;
        pimpl->rows_values = other.pimpl->rows_values;
        pimpl->columns_values = other.pimpl->columns_values;
        pimpl->levels_values = other.pimpl->levels_values;
        pimpl->seed_values = other.pimpl->seed_values;
        pimpl->algo_values = other.pimpl->algo_values;
        pimpl->distances_values = other.pimpl->distances_values;
        pimpl->help_flag = other.pimpl->help_flag;
        pimpl->version_flag = other.pimpl->version_flag;
        pimpl->distances_flag = other.pimpl->distances_flag;
    }
}

args& args::operator=(const args& other) {
    if (this == &other) {
        return *this;
    }
    
    if (!pimpl) {
        pimpl = std::make_unique<impl>();
    }
    
    if (other.pimpl) {
        pimpl->args_map = other.pimpl->args_map;
        pimpl->args_map_vec = other.pimpl->args_map_vec;
        
        pimpl->json_inputs = other.pimpl->json_inputs;
        pimpl->output_files = other.pimpl->output_files;
        pimpl->rows_values = other.pimpl->rows_values;
        pimpl->columns_values = other.pimpl->columns_values;
        pimpl->levels_values = other.pimpl->levels_values;  // Added missing levels_values
        pimpl->seed_values = other.pimpl->seed_values;
        pimpl->algo_values = other.pimpl->algo_values;
        pimpl->distances_values = other.pimpl->distances_values;
        pimpl->help_flag = other.pimpl->help_flag;
        pimpl->version_flag = other.pimpl->version_flag;
        pimpl->distances_flag = other.pimpl->distances_flag;
    }
    
    return *this;
}

void args::clear() noexcept {
    if (pimpl) {

        pimpl->clear();
    }
}

// Get a value from the args map by key
std::optional<std::string> args::get(const std::string& key) const noexcept {
    if (pimpl) {

        auto it = pimpl->args_map.find(key);
        if (it != pimpl->args_map.end()) {

            return it->second;
        }
    }

    return std::nullopt;
}

// Get entire args map
std::optional<std::unordered_map<std::string, std::string>> args::get() const noexcept {
    if (pimpl) {

        return std::make_optional(pimpl->args_map);
    }

    return std::nullopt;
}

// Get vector of args maps for JSON array parsing
std::optional<std::vector<std::unordered_map<std::string, std::string>>> args::get_array() const noexcept {
    if (pimpl) {
        return std::make_optional(pimpl->args_map_vec);
    }

    return std::nullopt;
}

// MAIN PARSE METHOD - All other parse methods funnel into this one
/// @brief Parse program arguments from a vector of strings
/// @param arguments Command-line arguments
/// @param has_program_name_as_first_arg Whether the first argument is the program name false
/// @return True if parsing was successful
bool args::parse(const std::vector<std::string>& arguments, bool has_program_name_as_first_arg) noexcept {

    if (arguments.empty()) {

        std::cerr << "No arguments provided to parse." << std::endl;

        return false;
    }
    
    // Determine which arguments to validate (skip program name if indicated)
    std::vector<std::string> validation_args = arguments;
    if (has_program_name_as_first_arg && !validation_args.empty()) {

        validation_args.erase(validation_args.begin());
    }
    
    // Pre-validate arguments before passing to CLI11
    if (!pimpl->pre_validate_arguments(validation_args)) {

        return false;
    }
        
    // Convert vector to argc/argv format for CLI11
    std::vector<const char*> argv_vec;
    if (!has_program_name_as_first_arg) {

        argv_vec.push_back("program");
    }
        
    for (const auto& arg : arguments) {

        argv_vec.push_back(arg.c_str());
    }
        
    int argc = static_cast<int>(argv_vec.size());
    const char** argv = argv_vec.data();
        
    // Use CLI11 to parse
    try {

        return this->pimpl->parse(argc, const_cast<char**>(argv));

    } catch (const std::exception& e) {

        std::cerr << "Arguments parsing error: " << e.what() << std::endl;

        this->clear();

        return false;
    }
}

// String parse method - funnels to vector parse
/// @brief Parse program arguments from a string
/// @param arguments Space-delimited command-line arguments
/// @param has_program_name_as_first_arg Whether the first argument is the program name false
/// @return True if parsing was successful
bool args::parse(const std::string& arguments, bool has_program_name_as_first_arg) noexcept {
        
    // Funnel to vector version
    try {

        // Split the string into a vector of arguments
        std::istringstream iss(arguments);
        std::vector<std::string> args_vector{ std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{} };

        return parse(args_vector, has_program_name_as_first_arg);
    } catch (const std::exception& e) {

        std::cerr << "Error parsing string arguments: " << e.what() << std::endl;

        this->clear();

        return false;
    }
}

// argc/argv parse method - funnels to vector parse
/// @brief Parse program arguments from argc/argv
/// @param argc Argument count
/// @param argv Argument values
/// @param has_program_name_as_first_arg Whether the first argument is the program name false
/// @return True if parsing was successful
bool args::parse(int argc, char** argv, bool has_program_name_as_first_arg) noexcept {
    try {

        // Convert argc/argv to vector of strings
        std::vector<std::string> args_vector;
        args_vector.reserve(argc);

        for (int i = 0; i < argc; ++i) {

            if (argv[i]) {

                args_vector.emplace_back(argv[i]);
            }
        }
        
        return parse(args_vector, has_program_name_as_first_arg);

    } catch (std::exception&) {

        std::cerr << "Error parsing argc/argv arguments:\n" << argc << "\n" << argv << std::endl;

        this->clear();

        return false;
    }
}

