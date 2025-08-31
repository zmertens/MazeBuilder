#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/string_utils.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <list>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <CLI11/CLI11.hpp>

using namespace mazes;

// Implementation class
class args::impl {

public:
    impl() : cli_app(DEFAULT_CLI_IMPLEMENTATION_NAME) {

        setup_cli();
    }
    
    // Storage for JSON array processing
    std::vector<std::unordered_map<std::string, std::string>> arguments;

    // Direct variable bindings for CLI11
    std::vector<std::string> algo_values;
    std::vector<int> columns_values;
    std::vector<std::string> distances_values;
    std::vector<std::string> json_inputs;
    std::vector<int> levels_values;
    std::vector<std::string> output_files;
    std::vector<int> rows_values;
    std::vector<int> seed_values;

    // Flag tracking
    bool help_flag = false;
    bool version_flag = false;
    bool distances_flag = false;

    // CLI11 app
    CLI::App cli_app;

private:
    static constexpr auto DEFAULT_CLI_IMPLEMENTATION_NAME = "MazeBuilderCommandLineInterface";

public:
    // Helper method to add argument variants for storing flags, options, and words
    void add_argument_variants(std::string_view key, std::string_view value) noexcept {

        using namespace std;

        unordered_map<string, string> local_args_map;
        
        if (key == args::ROW_WORD_STR) {

            local_args_map[args::ROW_FLAG_STR] = value;
            local_args_map[args::ROW_OPTION_STR] = value;
            local_args_map[args::ROW_WORD_STR] = value;
        } else if (key == args::COLUMN_WORD_STR) {

            local_args_map[args::COLUMN_FLAG_STR] = value;
            local_args_map[args::COLUMN_OPTION_STR] = value;
            local_args_map[args::COLUMN_WORD_STR] = value;
        } else if (key == args::LEVEL_WORD_STR) {

            local_args_map[args::LEVEL_FLAG_STR] = value;
            local_args_map[args::LEVEL_OPTION_STR] = value;
            local_args_map[args::LEVEL_WORD_STR] = value;
        } else if (key == args::SEED_WORD_STR) {

            local_args_map[args::SEED_FLAG_STR] = value;
            local_args_map[args::SEED_OPTION_STR] = value;
            local_args_map[args::SEED_WORD_STR] = value;
        } else if (key == args::OUTPUT_ID_WORD_STR) {

            local_args_map[args::OUTPUT_ID_FLAG_STR] = value;
            local_args_map[args::OUTPUT_ID_OPTION_STR] = value;
            local_args_map[args::OUTPUT_ID_WORD_STR] = value;
        } else if (key == args::JSON_WORD_STR) {

            local_args_map[args::JSON_FLAG_STR] = value;
            local_args_map[args::JSON_OPTION_STR] = value;
            local_args_map[args::JSON_WORD_STR] = value;
        } else if (key == args::DISTANCES_WORD_STR) {

            local_args_map[args::DISTANCES_FLAG_STR] = value;
            local_args_map[args::DISTANCES_OPTION_STR] = value;
            local_args_map[args::DISTANCES_WORD_STR] = value;
        } else if (key == args::ALGO_ID_WORD_STR) {

            local_args_map[args::ALGO_ID_FLAG_STR] = value;
            local_args_map[args::ALGO_ID_OPTION_STR] = value;
            local_args_map[args::ALGO_ID_WORD_STR] = value;
        } else if (key == args::HELP_WORD_STR) {

            local_args_map[args::HELP_FLAG_STR] = value;
            local_args_map[args::HELP_OPTION_STR] = value;
            local_args_map[args::HELP_WORD_STR] = value;
        } else if (key == args::VERSION_WORD_STR) {

            local_args_map[args::VERSION_FLAG_STR] = value;
            local_args_map[args::VERSION_OPTION_STR] = value;
            local_args_map[args::VERSION_WORD_STR] = value;
        } else {

            // For other keys, store as-is (like app name)
            if (!key.empty() && !(key[0] == '.' || key[0] == '/' || key[0] == '-')) {

                local_args_map.insert_or_assign(string{ key }, value);
            }
        }

        this->arguments.emplace_back(std::move(local_args_map));
    }

    // Validate slice syntax before processing
    bool validate_slice_syntax(const std::string& input) noexcept {

        using namespace std;

        if (auto splits = string_utils::split(input, ':'); !splits.empty()) {

            for (auto split_itr{splits.cbegin()}; split_itr != splits.cend(); ++split_itr) {

                if (split_itr->at(0) != '[') {

                    return false;
                }

                if (split_itr->at(split_itr->length() - 1) != ']') {

                    return false;
                }

                // Verify that every entry between brackets is a non-negative number
                auto content = split_itr->substr(1, split_itr->length() - 2);

                if (!all_of(content.cbegin(), content.cend(), ::isdigit) || content.empty()) {

                    return false;
                }
            }
        }
        
        return true;
    }

    // Pre-validate arguments before passing to CLI11
    bool pre_validate_arguments(const std::vector<std::string>& args) noexcept {

        using namespace std;

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
                if (auto eq_pos = arg.find('='); eq_pos != std::string::npos) {
                    
                    string option_part = arg.substr(0, eq_pos);

                    if (option_part == args::ROW_OPTION_STR || option_part == args::COLUMN_OPTION_STR ||
                        option_part == args::LEVEL_OPTION_STR || option_part == args::SEED_OPTION_STR ||
                        option_part == args::ALGO_ID_OPTION_STR || option_part == args::OUTPUT_ID_OPTION_STR ||
                        option_part == args::JSON_OPTION_STR || option_part == args::DISTANCES_OPTION_STR ||
                        option_part == args::HELP_OPTION_STR || option_part == args::VERSION_OPTION_STR) {

                        // Validate the value part for slice syntax if it's distances
                        if (option_part == args::DISTANCES_OPTION_STR) {

                            string value_part = arg.substr(eq_pos + 1);

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

                    if (short_opt == args::ALGO_ID_FLAG_STR[1] 
                        || short_opt == args::COLUMN_FLAG_STR[1]
                        || short_opt == args::DISTANCES_FLAG_STR[1]
                        || short_opt == args::HELP_FLAG_STR[1]
                        || short_opt == args::JSON_FLAG_STR[1]
                        || short_opt == args::LEVEL_FLAG_STR[1]
                        || short_opt == args::OUTPUT_ID_FLAG_STR[1]
                        || short_opt == args::ROW_FLAG_STR[1] 
                        || short_opt == args::SEED_FLAG_STR[1]
                        || short_opt == args::VERSION_FLAG_STR[1]) {

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
                        prev_arg == args::JSON_FLAG_STR || prev_arg == args::DISTANCES_FLAG_STR ||
                        prev_arg == args::HELP_FLAG_STR || prev_arg == args::VERSION_FLAG_STR) {
                        
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

private:
    void setup_cli() noexcept{

        using namespace std;

        // Configure CLI11 app to be more strict
        // Don't allow unknown arguments
        cli_app.allow_extras(false);
        cli_app.prefix_command(false);
        // Be case sensitive for better error detection
        cli_app.ignore_case(false);
        
        // Disable automatic help so we can handle it ourselves
        cli_app.set_help_flag();
        cli_app.set_version_flag();

        // Add options with direct vector binding

        auto ALGO_OPTIONS = string_utils::format("{},{}", args::ALGO_ID_FLAG_STR, args::ALGO_ID_OPTION_STR);
        cli_app.add_option(ALGO_OPTIONS, algo_values, "Algorithm to use for maze generation")
            ->capture_default_str();

        auto COLUMNS_OPTIONS = string_utils::format("{},{}", args::COLUMN_FLAG_STR, args::COLUMN_OPTION_STR);
        cli_app.add_option(COLUMNS_OPTIONS, columns_values, "Number of columns in the maze")
            ->capture_default_str();
            
        // Special handling for distances which can be flag or option with sliced array syntax
        auto DISTANCE_OPTIONS = string_utils::format("{},{}", args::DISTANCES_FLAG_STR, args::DISTANCES_OPTION_STR);
        cli_app.add_option(DISTANCE_OPTIONS, distances_values,
            "Calculate distances between cells, optionally with a range [start:end] where start and end are indices")
            ->expected(0, 1)
            ->capture_default_str();

        auto JSON_OPTIONS = string_utils::format("{},{}", args::JSON_FLAG_STR, args::JSON_OPTION_STR);
        cli_app.add_option(JSON_OPTIONS, json_inputs, "Parse JSON input file or string")
            ->capture_default_str();

        auto OUTPUT_OPTIONS = string_utils::format("{},{}", args::OUTPUT_ID_FLAG_STR, args::OUTPUT_ID_OPTION_STR);
        cli_app.add_option(OUTPUT_OPTIONS, output_files, "Output file")
            ->capture_default_str();

        auto ROWS_OPTIONS = string_utils::format("{},{}", args::ROW_FLAG_STR, args::ROW_OPTION_STR);
        cli_app.add_option(ROWS_OPTIONS, rows_values, "Number of rows in the maze")
            ->capture_default_str();

        auto LEVELS_OPTIONS = string_utils::format("{},{}", args::LEVEL_FLAG_STR, args::LEVEL_OPTION_STR);
        cli_app.add_option(LEVELS_OPTIONS, levels_values, "Number of levels in the maze")
            ->capture_default_str();

        auto SEED_OPTIONS = string_utils::format("{},{}", args::SEED_FLAG_STR, args::SEED_OPTION_STR);
        cli_app.add_option(SEED_OPTIONS, seed_values, "Random seed for maze generation")
            ->capture_default_str();

        // Add flags manually to avoid automatic exit behavior
        auto HELP_OPTIONS = string_utils::format("{},{}", args::HELP_FLAG_STR, args::HELP_OPTION_STR);
        cli_app.add_flag(HELP_OPTIONS, help_flag, "Show help information");

        auto VERSION_OPTIONS = string_utils::format("{},{}", args::VERSION_FLAG_STR, args::VERSION_OPTION_STR);
        cli_app.add_flag(VERSION_OPTIONS, version_flag, "Show version information");
    }

public:
    bool parse(int argc, char** argv, bool has_program_name_at_first_index = true) noexcept {

        // Convert to vector for pre-validation
        std::vector<std::string> args_vector;
        args_vector.reserve(argc);

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
            // Handle help and version requests and skip argument population
            if (e.get_exit_code() != 0) {

                return false;
            }

            populate_args_map();
        } catch (const std::exception& e) {

            // Catch any other exceptions and return false
            std::cerr << "Error parsing arguments: " << e.what() << std::endl;

            return false;
        }
        
        return true;
    }

    // Can throw exceptions
    void populate_args_map() {

        using namespace std;

        this->arguments.clear();

        // Store any extra arguments (like app names) as positional arguments
        if (auto extras{ cli_app.remaining() }; !extras.empty()) {

            // Store the first extra as the app name
            add_argument_variants(args::APP_KEY, extras[0]);
            // Store any other extras
            for (size_t i = 1; i < extras.size(); ++i) {

                add_argument_variants(string_utils::format("extra_{}", static_cast<int>(i)), extras[i]);
            }
        }
        
        // Handle JSON inputs and process them
        if (!json_inputs.empty()) {
            if (auto value = json_inputs.back(); !value.empty()) {

                add_argument_variants(args::JSON_WORD_STR, value);
                
                // Strip whitespace to determine if it's a JSON string vs file
                auto trimmed_value = string_utils::stripWhitespace(value);
                
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
        }
        
        // Handle output files
        if (!output_files.empty()) {
            if (auto value = output_files.back(); !value.empty()) {

                add_argument_variants(args::OUTPUT_ID_WORD_STR, value);
            }
        }
        
        // Handle rows
        if (!rows_values.empty()) {
            if (auto value = rows_values.back()) {

                add_argument_variants(args::ROW_WORD_STR, to_string(value));
            }
        }
        
        // Handle columns
        if (!columns_values.empty()) {
            if (auto value = columns_values.back()) {

                add_argument_variants(args::COLUMN_WORD_STR, to_string(value));
            }
        }
        
        // Handle levels
        if (!levels_values.empty()) {
            if (auto value = levels_values.back()) {

                add_argument_variants(args::LEVEL_WORD_STR, to_string(value));
            }
        }
        
        // Handle seed
        if (!seed_values.empty()) {
            if (auto value = seed_values.back()) {

                add_argument_variants(args::SEED_WORD_STR, to_string(value));
            }
        }
        
        // Handle algorithm
        if (!algo_values.empty()) {
            if (auto value = algo_values.back(); !value.empty()) {

                add_argument_variants(args::ALGO_ID_WORD_STR, value);
            }
        }
        
        // Handle distances (special case with sliced array parsing)
        if (!distances_values.empty()) {
            if (auto value = distances_values.back(); !value.empty()) {

                // Check if it looks like slice array syntax was used (contains colon)
                // If so, reconstruct the brackets that CLI11 strips off
                if (value.find(':') != string::npos && value.front() != '[') {

                    value = "[" + value + "]";
                }
                
                add_argument_variants(args::DISTANCES_WORD_STR, value);
                
                // Parse sliced array syntax
                parse_sliced_array(value);
               
            } else if (distances_flag || cli_app.count(args::DISTANCES_FLAG_STR) || cli_app.count(args::DISTANCES_OPTION_STR)) {

                // Flag form without value
                add_argument_variants(args::DISTANCES_WORD_STR, args::TRUE_VALUE);
            }
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

        using namespace std;

        try {

            // Remove backticks and parse JSON
            if (auto clean_json = string_utils::stripWhitespace(cref(json_str)); !clean_json.empty() 
                && clean_json.front() == '`' && clean_json.back() == '`') {

                clean_json = clean_json.substr(1, clean_json.length() - 2);

                // Use json_helper to parse the cleaned JSON string
                json_helper jh{};
                
                unordered_map<std::string, std::string> parsed_json;

                if (!jh.from(clean_json, parsed_json)) {

                    return false;
                }

                for (const auto& [key, value] : parsed_json) {
                    // Map JSON keys to argument keys and add using add_argument_variants
                    if (key == args::ROW_WORD_STR) {

                        add_argument_variants(args::ROW_WORD_STR, value);
                    } else if (key == args::COLUMN_WORD_STR) {

                        add_argument_variants(args::COLUMN_WORD_STR, value);
                    } else if (key == args::LEVEL_WORD_STR) {

                        add_argument_variants(args::LEVEL_WORD_STR, value);
                    } else if (key == args::SEED_WORD_STR) {

                        add_argument_variants(args::SEED_WORD_STR, value);
                    } else if (key == args::ALGO_ID_WORD_STR) {

                        add_argument_variants(args::ALGO_ID_WORD_STR, value);
                    } else if (key == args::OUTPUT_ID_WORD_STR) {

                        add_argument_variants(args::OUTPUT_ID_WORD_STR, value);
                    } else if (key == args::DISTANCES_WORD_STR) {

                        // Handle boolean distances field
                        if (value == args::TRUE_VALUE) {

                            add_argument_variants(args::DISTANCES_WORD_STR, args::TRUE_VALUE);
                        } else if (value != args::TRUE_VALUE) {

                            // Don't add distances if it's false
                        } else {

                            // Might be a slice notation as a string
                            add_argument_variants(args::DISTANCES_WORD_STR, value);

                            parse_sliced_array(value);
                        }
                    } else {

                        // Store unknown JSON keys as-is
                        arguments.emplace_back().insert_or_assign(key, value);
                    }
                }
            }
        } catch (const std::exception&) {

            return false;
        }

        return true;
    }
    
    // Internal JSON file processing for usage
    bool process_json_file(const std::string& filename) {
        
        using namespace std;
        
        try {
            
            // Check if file exists from current directory or tests directory
            auto test_file_path = filename;

            filesystem::path fp{ test_file_path };

            if (auto filepath{test_file_path}; !filesystem::exists(filepath)) {

                throw runtime_error{string_utils::format("File not found: {}", filepath)};
            }

            json_helper jh{};

            // First try to load as an array
            std::vector<std::unordered_map<std::string, std::string>> parsed_json_array;

            if (jh.load_array(cref(test_file_path), ref(parsed_json_array))) {
                
                // For backward compatibility, if there's only one object, also populate args_map

                if (const auto& first_object = parsed_json_array[0]; !parsed_json_array.empty()) {

                    for (const auto& [key, value] : first_object) {
                        // Map JSON keys to argument keys and add using add_argument_variants
                        if (key == args::ROW_WORD_STR) {

                            add_argument_variants(args::ROW_WORD_STR, value);
                        } else if (key == args::COLUMN_WORD_STR) {

                            add_argument_variants(args::COLUMN_WORD_STR, value);
                        } else if (key == args::LEVEL_WORD_STR) {

                            add_argument_variants(args::LEVEL_WORD_STR, value);
                        } else if (key == args::SEED_WORD_STR) {

                            add_argument_variants(args::SEED_WORD_STR, value);
                        } else if (key == args::ALGO_ID_WORD_STR) {

                            add_argument_variants(args::ALGO_ID_WORD_STR, value);
                        } else if (key == args::OUTPUT_ID_WORD_STR) {

                            add_argument_variants(args::OUTPUT_ID_WORD_STR, value);
                        } else if (key == args::DISTANCES_WORD_STR) {

                            // Handle boolean distances field
                            if (value == args::TRUE_VALUE) {
                                add_argument_variants(args::DISTANCES_WORD_STR, args::TRUE_VALUE);
                            } else if (value != args::TRUE_VALUE) {

                                // Don't add distances if it's false
                            } else {
                                // Might be a slice notation as a string
                                add_argument_variants(args::DISTANCES_WORD_STR, value);
                                parse_sliced_array(value);
                            }
                        } else {
                            
                            // Store unknown JSON keys as-is
                            this->arguments.emplace_back().insert_or_assign(key, value);
                        }
                    }
                }

                return true;
            } // load_array
            
            // If array loading failed, try loading as a single object (backward compatibility)
            std::unordered_map<std::string, std::string> parsed_json;

            if (jh.load(cref(test_file_path), ref(parsed_json))) {

                // Successfully parsed JSON file, now add the values to args_map
                for (const auto& [key, value] : parsed_json) {
                    // Map JSON keys to argument keys and add using add_argument_variants
                    if (key == args::ROW_WORD_STR) {

                        add_argument_variants(args::ROW_WORD_STR, value);
                    } else if (key == args::COLUMN_WORD_STR) {

                        add_argument_variants(args::COLUMN_WORD_STR, value);
                    } else if (key == args::LEVEL_WORD_STR) {

                        add_argument_variants(args::LEVEL_WORD_STR, value);
                    } else if (key == args::SEED_WORD_STR) {

                        add_argument_variants(args::SEED_WORD_STR, value);
                    } else if (key == args::ALGO_ID_WORD_STR) {

                        add_argument_variants(args::ALGO_ID_WORD_STR, value);
                    } else if (key == args::OUTPUT_ID_WORD_STR) {

                        add_argument_variants(args::OUTPUT_ID_WORD_STR, value);
                    } else if (key == args::DISTANCES_WORD_STR) {

                        // Handle boolean distances field
                        if (value == args::TRUE_VALUE) {
                            add_argument_variants(args::DISTANCES_WORD_STR, args::TRUE_VALUE);
                        } else if (value != args::TRUE_VALUE) {

                            // Don't add distances if it's false
                        } else {
                            // Might be a slice notation as a string
                            add_argument_variants(args::DISTANCES_WORD_STR, value);
                            parse_sliced_array(value);
                        }
                    } else {
                        
                        // Store unknown JSON keys as-is
                        this->arguments.emplace_back().insert_or_assign(key, value);
                    }
                }

                return true;
            } // load
        } catch (const std::exception& ex) {

            throw runtime_error{string_utils::format("Error processing JSON file {}: {}", filename, ex.what())};
        }

        return false;
    }

    void clear() noexcept {

        arguments.clear();

        // Clear vectors in impl
        algo_values.clear();
        columns_values.clear();
        distances_values.clear();
        json_inputs.clear();
        levels_values.clear();
        output_files.clear();
        rows_values.clear();
        seed_values.clear();

        // Reset flags
        help_flag = false;
        version_flag = false;
        distances_flag = false;
    }

    // Public method for external access to slice parsing
    bool parse_sliced_array(const std::string& value) noexcept {

        using namespace std;

        regex slice_pattern(R"(\[(.*?):(.*?)\])");

        smatch matches;

        if (regex_match(value, matches, slice_pattern)) {

            auto start_idx = matches[1].str();

            auto end_idx = matches[2].str();

            // Store the parsed start and end indices
            if (!start_idx.empty()) {

                arguments.emplace_back().insert_or_assign(args::DISTANCES_START_STR, start_idx);
            } else {
                arguments.emplace_back().insert_or_assign(args::DISTANCES_START_STR, std::to_string(configurator::DEFAULT_DISTANCES_START));
            }

            if (!end_idx.empty()) {

                arguments.emplace_back().insert_or_assign(args::DISTANCES_END_STR, end_idx);
            } else {

                arguments.emplace_back().insert_or_assign(args::DISTANCES_END_STR, std::to_string(configurator::DEFAULT_DISTANCES_END));
            }

            return true;
        }

        return false;
    }
}; // impl

args::args() noexcept : pimpl{ std::make_unique<impl>() } {
}

args::~args() = default;

args::args(const args& other) : pimpl{ std::make_unique<impl>() } {
    
    if (other.pimpl) {

        pimpl->arguments = other.pimpl->arguments;
        
        pimpl->algo_values = other.pimpl->algo_values;
        pimpl->columns_values = other.pimpl->columns_values;
        pimpl->distances_flag = other.pimpl->distances_flag;
        pimpl->distances_values = other.pimpl->distances_values;
        pimpl->help_flag = other.pimpl->help_flag;
        pimpl->json_inputs = other.pimpl->json_inputs;
        pimpl->levels_values = other.pimpl->levels_values;
        pimpl->output_files = other.pimpl->output_files;
        pimpl->rows_values = other.pimpl->rows_values;
        pimpl->seed_values = other.pimpl->seed_values;
        pimpl->version_flag = other.pimpl->version_flag;
    }
}

args& args::operator=(const args& other) {

    if (this == &other) {

        return *this;
    }
    
    pimpl = exchange(pimpl, std::make_unique<impl>());
    
    if (other.pimpl) {

        pimpl->arguments = other.pimpl->arguments;

        pimpl->algo_values = other.pimpl->algo_values;
        pimpl->columns_values = other.pimpl->columns_values;
        pimpl->distances_flag = other.pimpl->distances_flag;
        pimpl->distances_values = other.pimpl->distances_values;
        pimpl->help_flag = other.pimpl->help_flag;
        pimpl->json_inputs = other.pimpl->json_inputs;
        pimpl->levels_values = other.pimpl->levels_values;
        pimpl->output_files = other.pimpl->output_files;
        pimpl->rows_values = other.pimpl->rows_values;
        pimpl->seed_values = other.pimpl->seed_values;
        pimpl->version_flag = other.pimpl->version_flag;
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

    if (pimpl->arguments.empty()) {
        return std::nullopt;
    }

    if (auto it = pimpl->arguments.front().find(key); it != pimpl->arguments.front().end()) {

        return it->second;
    }

    return std::nullopt;
}

// Get entire args map
std::optional<std::unordered_map<std::string, std::string>> args::get() const noexcept {
    
    if (pimpl->arguments.empty()) {
        return std::nullopt;
    }
    
    return std::make_optional(pimpl->arguments.front());
}

// Get vector of args maps for JSON array parsing
std::optional<std::vector<std::unordered_map<std::string, std::string>>> args::get_array() const noexcept {

    return std::make_optional(pimpl->arguments);
}

// MAIN PARSE METHOD - All other parse methods funnel into this one
/// @brief Parse program arguments from a vector of strings
/// @param arguments Command-line arguments
/// @param has_program_name_as_first_arg Whether the first argument is the program name false
/// @return True if parsing was successful
bool args::parse(const std::vector<std::string>& arguments, bool has_program_name_as_first_arg) noexcept {

    using namespace std;

    // Determine which arguments to validate (skip program name if indicated)
    if (auto validation_args{arguments}; !validation_args.empty()) {

        if (has_program_name_as_first_arg) {
         
            validation_args.erase(validation_args.begin());
        }

        // Pre-validate arguments before passing to internal parser
        if (!pimpl->pre_validate_arguments(cref(validation_args))) {

            return false;
        }

        // Convert vector to argc/argv format for internal parser
        vector<const char*> argv_vec;
            
        for (const auto& arg : validation_args) {

            argv_vec.push_back(arg.c_str());
        }
        
        int argc = static_cast<int>(argv_vec.size());
        const char** argv = argv_vec.data();
        
        // Special case: if we removed the program name and have no arguments left,
        // CLI11 might have issues with argc=0. In this case, we know it's just 
        // the app name, so we can directly populate and return success.
        if (has_program_name_as_first_arg && argc == 0) {
            // Only app name was provided, populate it directly
            pimpl->arguments.clear();
            pimpl->add_argument_variants(args::APP_KEY, arguments.front());
            return true;
        }
        
        // Use CLI11 to parse
        try {

            return this->pimpl->parse(argc, const_cast<char**>(argv), false);

        } catch (const std::exception& e) {

            cerr << "Arguments parsing error: " << e.what() << endl;

            this->clear();

            return false;
        }
    }

    return false;
}

// String parse method - funnels to vector parse
/// @brief Parse program arguments from a string
/// @param arguments Space-delimited command-line arguments
/// @param has_program_name_as_first_arg Whether the first argument is the program name false
/// @return True if parsing was successful
bool args::parse(const std::string& arguments, bool has_program_name_as_first_arg) noexcept {
        
    using namespace std;

    // Funnel to vector version
    try {

        // Split the string into a vector of arguments
        istringstream iss(arguments);

        vector<string> args_vector{ istream_iterator<string>{iss}, istream_iterator<string>{} };

        return parse(args_vector, has_program_name_as_first_arg);
    } catch (const std::exception& e) {

        cerr << "Error parsing string arguments: " << e.what() << endl;

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

    using namespace std;

    try {

        // Convert argc/argv to vector of strings
        vector<string> args_vector;

        args_vector.reserve(argc);

        for (int i = 0; i < argc; ++i) {

            if (argv[i]) {

                args_vector.emplace_back(argv[i]);
            }
        }
        
        return parse(args_vector, has_program_name_as_first_arg);

    } catch (std::exception&) {

        cerr << "Error parsing argc/argv arguments:\n" << argc << "\n" << argv << endl;

        this->clear();

        return false;
    }
}

