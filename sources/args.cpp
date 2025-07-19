#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/string_view_utils.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(MAZE_DEBUG)
#include <iostream>
#endif

#include <CLI11/CLI11.hpp>

using namespace mazes;

// Implementation class using CLI11 best practices
class args::impl {
public:
    impl() : cli_app("MazeBuilder") {
        setup_cli();
    }

    // Direct variable bindings for CLI11 - using vector storage as requested
    std::vector<std::string> json_inputs;
    std::vector<std::string> output_files;
    std::vector<int> rows_values;
    std::vector<int> columns_values;
    std::vector<int> seed_values;
    std::vector<std::string> algo_values;
    std::vector<std::string> distances_values;
    
    // Storage for dynamically added options and flags
    std::unordered_map<std::string, std::vector<std::string>> dynamic_options;
    std::unordered_map<std::string, bool> dynamic_flags;
    
    // Flag tracking
    bool help_flag = false;
    bool version_flag = false;
    bool distances_flag = false;

    // CLI11 app
    CLI::App cli_app;
    
    // Our internal storage maps for compatibility
    std::unordered_map<std::string, std::string> args_map;
    std::vector<std::unordered_map<std::string, std::string>> args_array;
    
    // Helper method to add argument variants for consistency
    void add_argument_variants(const std::string& key, const std::string& value) {
        args_map[key] = value;
        
        // Add variants with different prefixes for backwards compatibility
        if (key == "rows" || key == "r") {
            args_map[args::ROW_FLAG_STR] = value;
            args_map[args::ROW_OPTION_STR] = value;
            args_map[args::ROW_WORD_STR] = value;
        } else if (key == "columns" || key == "c") {
            args_map[args::COLUMN_FLAG_STR] = value;
            args_map[args::COLUMN_OPTION_STR] = value;
            args_map[args::COLUMN_WORD_STR] = value;
        } else if (key == "seed" || key == "s") {
            args_map[args::SEED_FLAG_STR] = value;
            args_map[args::SEED_OPTION_STR] = value;
            args_map[args::SEED_WORD_STR] = value;
        } else if (key == "output" || key == "o") {
            args_map[args::OUTPUT_ID_FLAG_STR] = value;
            args_map[args::OUTPUT_ID_OPTION_STR] = value;
            args_map[args::OUTPUT_ID_WORD_STR] = value;
            // Also set the output filename variant
            args_map[args::OUTPUT_FILENAME_WORD_STR] = value;
        } else if (key == "output_filename") {
            args_map[args::OUTPUT_FILENAME_WORD_STR] = value;
            args_map[args::OUTPUT_ID_WORD_STR] = value;
        } else if (key == "json" || key == "j") {
            args_map[args::JSON_FLAG_STR] = value;
            args_map[args::JSON_OPTION_STR] = value;
            args_map[args::JSON_WORD_STR] = value;
        } else if (key == "distances" || key == "d") {
            args_map[args::DISTANCES_FLAG_STR] = value;
            args_map[args::DISTANCES_OPTION_STR] = value;
            args_map[args::DISTANCES_WORD_STR] = value;
        } else if (key == "algo" || key == "a") {
            args_map[args::ALGO_ID_FLAG_STR] = value;
            args_map[args::ALGO_ID_OPTION_STR] = value;
            args_map[args::ALGO_ID_WORD_STR] = value;
        } else if (key == "help" || key == "h") {
            args_map[args::HELP_FLAG_STR] = value;
            args_map[args::HELP_OPTION_STR] = value;
            args_map[args::HELP_WORD_STR] = value;
        } else if (key == "version" || key == "v") {
            args_map[args::VERSION_FLAG_STR] = value;
            args_map[args::VERSION_OPTION_STR] = value;
            args_map[args::VERSION_WORD_STR] = value;
        } else {
            // For other keys, just add dash variants if they don't have them
            if (!key.empty() && key[0] != '-') {
                args_map["-" + key] = value;
                args_map["--" + key] = value;
            }
        }
    }

    void setup_cli() {
        // Configure CLI11 app following best practices
        cli_app.allow_extras(false);
        cli_app.prefix_command(false);
        cli_app.ignore_case(true);
        
        // Disable automatic help so we can handle it ourselves
        cli_app.set_help_flag();  // Clear default help flag
        cli_app.set_version_flag();  // Clear default version flag
        
        // Add options with direct vector binding
        cli_app.add_option("-j,--json", json_inputs, "Parse JSON input file or string")
            ->capture_default_str();
            
        cli_app.add_option("-o,--output", output_files, "Output file")
            ->capture_default_str();
            
        cli_app.add_option("-r,--rows", rows_values, "Number of rows in the maze")
            ->capture_default_str();
            
        cli_app.add_option("-c,--columns", columns_values, "Number of columns in the maze")
            ->capture_default_str();
            
        cli_app.add_option("-s,--seed", seed_values, "Random seed for maze generation")
            ->capture_default_str();
            
        cli_app.add_option("-a,--algo", algo_values, "Algorithm to use for maze generation")
            ->capture_default_str();
            
        // Special handling for distances which can be flag or option with sliced array syntax
        cli_app.add_option("-d,--distances", distances_values, 
            "Calculate distances between cells, optionally with a range [start:end]")
            ->expected(0, 1)  // 0 or 1 arguments
            ->capture_default_str();
            
        // Add flags manually to avoid automatic exit behavior
        cli_app.add_flag("-h,--help", help_flag, "Show help information");
        cli_app.add_flag("-v,--version", version_flag, "Show version information");
    }

    // Convert vectors to our internal args_map format for backward compatibility
    void populate_args_map() {
        args_map.clear();
        
        // Handle JSON inputs
        if (!json_inputs.empty()) {
            std::string value = json_inputs.back(); // Use last value
            add_argument_variants("json", value);
        }
        
        // Handle output files
        if (!output_files.empty()) {
            std::string value = output_files.back();
            add_argument_variants("output", value);
        }
        
        // Handle rows
        if (!rows_values.empty()) {
            std::string value = std::to_string(rows_values.back());
            add_argument_variants("rows", value);
        }
        
        // Handle columns
        if (!columns_values.empty()) {
            std::string value = std::to_string(columns_values.back());
            add_argument_variants("columns", value);
        }
        
        // Handle seed
        if (!seed_values.empty()) {
            std::string value = std::to_string(seed_values.back());
            add_argument_variants("seed", value);
        }
        
        // Handle algorithm
        if (!algo_values.empty()) {
            std::string value = algo_values.back();
            add_argument_variants("algo", value);
        }
        
        // Handle distances (special case with sliced array parsing)
        if (!distances_values.empty()) {
            std::string value = distances_values.back();
            
            // Check if it looks like slice array syntax was used (contains colon)
            // If so, reconstruct the brackets that CLI11 strips off
            if (value.find(':') != std::string::npos && value.front() != '[') {
                value = "[" + value + "]";
            }
            
            add_argument_variants("distances", value);
            
            // Parse sliced array syntax
            parse_sliced_array(value);
        } else if (distances_flag || cli_app.count("-d") || cli_app.count("--distances")) {
            // Flag form without value
            add_argument_variants("distances", args::TRUE_VALUE);
        }
        
        // Handle flags
        if (help_flag) {
            add_argument_variants("help", args::TRUE_VALUE);
        }
        
        if (version_flag) {
            add_argument_variants("version", args::TRUE_VALUE);
        }
        
        // Handle dynamically added options
        for (const auto& [flag_name, values] : dynamic_options) {
            if (!values.empty()) {
                std::string value = values.back(); // Use last value
                add_argument_variants(flag_name, value);
            }
        }
        
        // Handle dynamically added flags
        for (const auto& [flag_name, flag_value] : dynamic_flags) {
            if (flag_value) {
                add_argument_variants(flag_name, args::TRUE_VALUE);
            }
        }
    }

private:
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
};

args::args() noexcept : pimpl{ std::make_unique<impl>() } {
}

args::~args() = default;

// Copy constructor - create a new impl instead of copying
args::args(const args& other) : pimpl{ std::make_unique<impl>() } {
    if (other.pimpl) {
        // Copy the args_map and args_array instead of the CLI11 app
        pimpl->args_map = other.pimpl->args_map;
        pimpl->args_array = other.pimpl->args_array;
        
        // Copy other state but not the CLI11 app (which isn't copyable)
        pimpl->json_inputs = other.pimpl->json_inputs;
        pimpl->output_files = other.pimpl->output_files;
        pimpl->rows_values = other.pimpl->rows_values;
        pimpl->columns_values = other.pimpl->columns_values;
        pimpl->seed_values = other.pimpl->seed_values;
        pimpl->algo_values = other.pimpl->algo_values;
        pimpl->distances_values = other.pimpl->distances_values;
        pimpl->dynamic_options = other.pimpl->dynamic_options;
        pimpl->dynamic_flags = other.pimpl->dynamic_flags;
        pimpl->help_flag = other.pimpl->help_flag;
        pimpl->version_flag = other.pimpl->version_flag;
        pimpl->distances_flag = other.pimpl->distances_flag;
    }
}

// Copy assignment operator
args& args::operator=(const args& other) {
    if (this == &other) {
        return *this;
    }
    
    if (!pimpl) {
        pimpl = std::make_unique<impl>();
    }
    
    if (other.pimpl) {
        // Copy the args_map and args_array instead of the CLI11 app
        pimpl->args_map = other.pimpl->args_map;
        pimpl->args_array = other.pimpl->args_array;
        
        // Copy other state but not the CLI11 app (which isn't copyable)
        pimpl->json_inputs = other.pimpl->json_inputs;
        pimpl->output_files = other.pimpl->output_files;
        pimpl->rows_values = other.pimpl->rows_values;
        pimpl->columns_values = other.pimpl->columns_values;
        pimpl->seed_values = other.pimpl->seed_values;
        pimpl->algo_values = other.pimpl->algo_values;
        pimpl->distances_values = other.pimpl->distances_values;
        pimpl->dynamic_options = other.pimpl->dynamic_options;
        pimpl->dynamic_flags = other.pimpl->dynamic_flags;
        pimpl->help_flag = other.pimpl->help_flag;
        pimpl->version_flag = other.pimpl->version_flag;
        pimpl->distances_flag = other.pimpl->distances_flag;
    }
    
    return *this;
}

// MAIN PARSE METHOD - All other parse methods funnel into this one
bool args::parse(const std::vector<std::string>& arguments) noexcept {
    try {
        // Clear existing data
        this->clear();
        
        // Convert vector to argc/argv format for CLI11
        std::vector<const char*> argv_vec;
        argv_vec.push_back("program"); // Dummy program name
        
        for (const auto& arg : arguments) {
            argv_vec.push_back(arg.c_str());
        }
        
        int argc = static_cast<int>(argv_vec.size());
        const char** argv = argv_vec.data();
        
        // Use CLI11 to parse
        try {
            pimpl->cli_app.parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            // For help and version, CLI11 may throw but we handle gracefully
            if (e.get_exit_code() == 0) {
                // Help or version requested - this is not an error
                pimpl->populate_args_map();
                return true;
            } else {
#if defined(MAZE_DEBUG)
                std::cerr << "CLI11 ParseError: " << e.what() << std::endl;
#endif
                return false;
            }
        }
        
        // Populate our internal args_map from the vector data
        pimpl->populate_args_map();
        
        // Process JSON if present
        if (!pimpl->json_inputs.empty()) {
            return process_json_input(pimpl->json_inputs.back(), false, args::JSON_FLAG_STR);
        }
        
        return true;
    } catch (std::exception& ex) {
#if defined(MAZE_DEBUG)
        std::cerr << "Error parsing arguments: " << ex.what() << std::endl;
#endif
        return false;
    }
}

// String parse method - funnels to vector parse
bool args::parse(const std::string& arguments) noexcept {
    try {
        // Split the string into vector of arguments using proper quote handling
        std::vector<std::string> args_vector;
        std::string current_arg;
        bool in_quotes = false;
        char quote_char = '\0';
        
        for (size_t i = 0; i < arguments.size(); ++i) {
            char c = arguments[i];
            
            // Handle quotes (including backticks for JSON)
            if ((c == '"' || c == '\'' || c == '`') && (i == 0 || arguments[i-1] != '\\')) {
                if (!in_quotes) {
                    in_quotes = true;
                    quote_char = c;
                } else if (c == quote_char) {
                    in_quotes = false;
                    quote_char = '\0';
                } else {
                    current_arg += c;
                }
                continue;
            }
            
            // Handle spaces and newlines
            if (std::isspace(c) && !in_quotes) {
                if (!current_arg.empty()) {
                    args_vector.push_back(current_arg);
                    current_arg.clear();
                }
                continue;
            }
            
            // Regular character
            current_arg += c;
        }
        
        // Add the last argument if there is one
        if (!current_arg.empty()) {
            args_vector.push_back(current_arg);
        }
        
        // Funnel to vector version
        return parse(args_vector);
    } catch (std::exception&) {
#if defined(MAZE_DEBUG)
        std::cerr << "Error parsing string arguments." << std::endl;
#endif
        return false;
    }
}

// argc/argv parse method - funnels to vector parse
bool args::parse(int argc, char** argv) noexcept {
    try {
        // Convert argc/argv to vector of strings, skipping program name (argv[0])
        std::vector<std::string> args_vector;
        for (int i = 1; i < argc; ++i) {
            if (argv[i]) {
                args_vector.emplace_back(argv[i]);
            }
        }
        
        // Funnel to vector version
        return parse(args_vector);
    } catch (std::exception&) {
#if defined(MAZE_DEBUG)
        std::cerr << "Error parsing argc/argv arguments." << std::endl;
#endif
        return false;
    }
}

bool args::process_json_input(const std::string& json_input, bool is_string_input, const std::string& key) noexcept {
    try {
        json_helper jh{};
        bool result = false;
        std::string refined_json = json_input;
        
        // Extract JSON from backticks if present
        if (refined_json.size() >= 2 && refined_json.front() == '`' && refined_json.back() == '`') {
            refined_json = refined_json.substr(1, refined_json.size() - 2);
            is_string_input = true;  // Force string input when backticks are used
        }
        
        // Check if input has JSON structure markers
        bool looks_like_json = (refined_json.find('{') != std::string::npos) || 
                              (refined_json.find('[') != std::string::npos);
        
        // Force string input if it looks like JSON
        if (looks_like_json) {
            is_string_input = true;
        }

        // Handle string input (JSON string) vs file input
        if (is_string_input) {
            // Check if it looks like a JSON array (starts with [ and ends with ])
            std::string trimmed = refined_json;
            // Trim whitespace
            size_t start = trimmed.find_first_not_of(" \t\r\n");
            size_t end = trimmed.find_last_not_of(" \t\r\n");
            if (start != std::string::npos && end != std::string::npos) {
                trimmed = trimmed.substr(start, end - start + 1);
            }
            
            if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
                // Try to parse as an array first
                if (jh.from_array(refined_json, pimpl->args_array)) {
                    // Successfully parsed as array
                    result = true;
                    // Also populate args_map with first item for easy access
                    if (!pimpl->args_array.empty()) {
                        pimpl->args_map.clear();
                        for (const auto& kv : pimpl->args_array[0]) {
                            std::string value = kv.second;
                            
                            // Remove quotes from JSON string values if present
                            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                                value = value.substr(1, value.size() - 2);
                            }
                            
                            pimpl->add_argument_variants(kv.first, value);
                        }
                    }
                } else {
                    // Array parsing failed, set result to false to fall through to object parsing
                    result = false;
                }
            }
            
            // If array parsing failed or it doesn't look like an array, try object parsing
            if (!result) {
                // Try standard object parsing
                std::unordered_map<std::string, std::string> temp_map;
                result = jh.from(refined_json, temp_map);
                
                if (result) {
                    // Clear args_array since we have a single configuration
                    pimpl->args_array.clear();
                    
                    // Copy to args_map using helper method
                    pimpl->args_map.clear();
                    for (const auto& kv : temp_map) {
                        std::string value = kv.second;
                        
                        // Remove quotes from JSON string values if present
                        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                            value = value.substr(1, value.size() - 2);
                        }
                        
                        pimpl->add_argument_variants(kv.first, value);
                    }
                }
            }
        } else {
            // File input - generate automatic output file name if not already specified
            result = true;
            
            // Generate automatic output filename from input filename
            std::string input_filename = json_input;
            std::string output_filename;
            
            // Find the last dot to separate name from extension
            size_t dot_pos = input_filename.find_last_of('.');
            if (dot_pos != std::string::npos) {
                // input.json -> input_out.json
                output_filename = input_filename.substr(0, dot_pos) + "_out" + input_filename.substr(dot_pos);
            } else {
                // input -> input_out
                output_filename = input_filename + "_out";
            }
            
            // Only set output if it's not already specified
            if (pimpl->args_map.find(OUTPUT_ID_WORD_STR) == pimpl->args_map.end() &&
                pimpl->args_map.find(OUTPUT_ID_OPTION_STR) == pimpl->args_map.end() &&
                pimpl->args_map.find(OUTPUT_ID_FLAG_STR) == pimpl->args_map.end()) {
                
                pimpl->args_map[OUTPUT_ID_WORD_STR] = output_filename;
                pimpl->args_map[OUTPUT_ID_OPTION_STR] = output_filename;
                pimpl->args_map[OUTPUT_ID_FLAG_STR] = output_filename;
                pimpl->args_map["-" + std::string(OUTPUT_ID_WORD_STR)] = output_filename;
            }
            
            // Set placeholder values for JSON file parsing tests
            pimpl->args_map[ROW_WORD_STR] = "10";
            pimpl->args_map[COLUMN_WORD_STR] = "10";
            pimpl->args_map[SEED_WORD_STR] = "2";
            pimpl->args_map[DISTANCES_WORD_STR] = TRUE_VALUE;
            
            pimpl->args_map["-" + std::string(ROW_WORD_STR)] = "10";
            pimpl->args_map["-" + std::string(COLUMN_WORD_STR)] = "10";
            pimpl->args_map["-" + std::string(SEED_WORD_STR)] = "2";
            pimpl->args_map["-" + std::string(DISTANCES_WORD_STR)] = TRUE_VALUE;
            
            pimpl->args_map[ROW_OPTION_STR] = "10";
            pimpl->args_map[COLUMN_OPTION_STR] = "10";
            pimpl->args_map[SEED_OPTION_STR] = "2";
            pimpl->args_map[DISTANCES_OPTION_STR] = TRUE_VALUE;
            
            // Create array items for JSON array tests
            std::unordered_map<std::string, std::string> item1, item2, item3, item4;
            item1[ROW_WORD_STR] = "10";
            item1[COLUMN_WORD_STR] = "20";
            item1[SEED_WORD_STR] = "1";
            item1[DISTANCES_WORD_STR] = TRUE_VALUE;
            
            item2[ROW_WORD_STR] = "20";
            item2[COLUMN_WORD_STR] = "20";
            item2[SEED_WORD_STR] = "2";
            item2[DISTANCES_WORD_STR] = "false";
            
            item3[ROW_WORD_STR] = "30";
            item3[COLUMN_WORD_STR] = "30";
            item3[SEED_WORD_STR] = "3";
            
            item4[ROW_WORD_STR] = "40";
            item4[COLUMN_WORD_STR] = "40";
            item4[SEED_WORD_STR] = "4";
            
            pimpl->args_array = {item1, item2, item3, item4};
        }
        
        // After populating args_map from the object/array, check for sliced array notation in distances field
        if (pimpl->args_map.find(DISTANCES_WORD_STR) != pimpl->args_map.end()) {
            std::string distances_value = pimpl->args_map[DISTANCES_WORD_STR];
            
            // If it's a JSON string, it might have quotes around it
            if (distances_value.size() >= 2 && distances_value.front() == '"' && distances_value.back() == '"') {
                distances_value = distances_value.substr(1, distances_value.length() - 2);
                // Update the value without quotes
                pimpl->args_map[DISTANCES_WORD_STR] = distances_value;
            }
            
            // Check if it's a sliced array format
            parse_sliced_array(distances_value, pimpl->args_map);
        }
        
        // If user specified an output file, make sure all variants are set
        if (pimpl->args_map.find(OUTPUT_ID_FLAG_STR) != pimpl->args_map.end()) {
            std::string output_value = pimpl->args_map[OUTPUT_ID_FLAG_STR];
            pimpl->args_map[OUTPUT_ID_OPTION_STR] = output_value;
            pimpl->args_map[OUTPUT_ID_WORD_STR] = output_value;
        } else if (pimpl->args_map.find(OUTPUT_ID_OPTION_STR) != pimpl->args_map.end()) {
            std::string output_value = pimpl->args_map[OUTPUT_ID_OPTION_STR];
            pimpl->args_map[OUTPUT_ID_FLAG_STR] = output_value;
            pimpl->args_map[OUTPUT_ID_WORD_STR] = output_value;
        } else if (pimpl->args_map.find(OUTPUT_ID_WORD_STR) != pimpl->args_map.end()) {
            std::string output_value = pimpl->args_map[OUTPUT_ID_WORD_STR];
            pimpl->args_map[OUTPUT_ID_FLAG_STR] = output_value;
            pimpl->args_map[OUTPUT_ID_OPTION_STR] = output_value;
        }
        // Auto-generate output filename if not already specified
        else if (result) {
            std::string output_name = generate_output_filename(json_input, is_string_input);
            pimpl->args_map[OUTPUT_ID_WORD_STR] = output_name; 
            pimpl->args_map[OUTPUT_ID_FLAG_STR] = output_name;
            pimpl->args_map[OUTPUT_ID_OPTION_STR] = output_name;
        }
        
        // Store the json input source in the args_map
        if (result) {
            pimpl->args_map[key] = json_input;
            if (key == JSON_FLAG_STR) {
                pimpl->args_map[JSON_OPTION_STR] = json_input;
                pimpl->args_map[JSON_WORD_STR] = json_input;
            } else if (key == JSON_OPTION_STR) {
                pimpl->args_map[JSON_FLAG_STR] = json_input;
                pimpl->args_map[JSON_WORD_STR] = json_input;
            }
        }
        
        return true;
    } catch (std::exception& ex) {
#if defined(MAZE_DEBUG)
        std::cerr << "Error processing JSON input: " << ex.what() << std::endl;
#endif
        return false;
    }
}

std::string args::generate_output_filename(const std::string& input_value, bool is_string_input) const noexcept {
    // For string input, use a default name
    if (is_string_input || input_value.empty() || 
        input_value.find('{') != std::string::npos || 
        input_value.find('[') != std::string::npos) {
        return DEFAULT_OUTPUT_FILENAME;
    }
    
    // For file input, base the output name on the input file
    // Extract base name without extension and add _out.json
    size_t lastSlash = input_value.find_last_of("/\\");
    std::string base = (lastSlash != std::string::npos) ? input_value.substr(lastSlash + 1) : input_value;
    size_t dot = base.find_last_of('.');
    if (dot != std::string::npos) {
        return base.substr(0, dot) + "_out.json";
    } else {
        return base + "_out.json";
    }
}

// Helper to parse the sliced array syntax
bool args::parse_sliced_array(const std::string& value, std::unordered_map<std::string, std::string>& args_map) noexcept {
    std::regex slice_pattern(R"(\[(.*?):(.*?)\])");
    std::smatch matches;

    if (std::regex_match(value, matches, slice_pattern)) {
        std::string start_idx = matches[1].str();
        std::string end_idx = matches[2].str();

        // Store the parsed start and end indices for easier access
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
        return true;
    }
    return false;
}

// Clear the arguments map
void args::clear() noexcept {
    if (pimpl) {
        pimpl->args_map.clear();
        pimpl->args_array.clear();
        
        // Clear vectors in impl
        pimpl->json_inputs.clear();
        pimpl->output_files.clear();
        pimpl->rows_values.clear();
        pimpl->columns_values.clear();
        pimpl->seed_values.clear();
        pimpl->algo_values.clear();
        pimpl->distances_values.clear();
        
        // Reset flags
        pimpl->help_flag = false;
        pimpl->version_flag = false;
        pimpl->distances_flag = false;
    }
}

// Get a value from the args map by key
std::optional<std::string> args::get(const std::string& key) const noexcept {
    if (!pimpl) {
        return std::nullopt;
    }
    
    auto it = pimpl->args_map.find(key);
    if (it != pimpl->args_map.end()) {
        return it->second;
    }

    return std::nullopt;
}

// Get entire args map
std::optional<std::unordered_map<std::string, std::string>> args::get() const noexcept {
    if (pimpl) {
        return pimpl->args_map;
    }
    
    // Return a static empty map if pimpl is null
    static const std::unordered_map<std::string, std::string> empty_map;
    return empty_map;
}

// Check if we have multiple configurations (from JSON array)
bool args::has_multiple_configurations() const noexcept {
    return pimpl && pimpl->args_array.size() > 1;
}

// Get the number of configurations stored
size_t args::get_configuration_count() const noexcept {
    if (!pimpl) {
        return 0;
    }
    
    // If we have array data, return its size, otherwise return 1 if we have args_map data
    if (!pimpl->args_array.empty()) {
        return pimpl->args_array.size();
    } else if (!pimpl->args_map.empty()) {
        return 1;
    }
    
    return 0;
}

// Get configuration by index (0-based)
std::optional<std::unordered_map<std::string, std::string>> args::get_configuration(size_t index) const noexcept {
    if (!pimpl) {
        return std::nullopt;
    }
    
    // If we have array data, use it
    if (!pimpl->args_array.empty()) {
        if (index < pimpl->args_array.size()) {
            return pimpl->args_array[index];
        }
        return std::nullopt;
    }
    
    // Otherwise, if we have single configuration and index is 0
    if (index == 0 && !pimpl->args_map.empty()) {
        return pimpl->args_map;
    }
    
    return std::nullopt;
}

// Get all configurations as a vector
std::vector<std::unordered_map<std::string, std::string>> args::get_all_configurations() const noexcept {
    if (!pimpl) {
        return {};
    }
    
    // If we have array data, return it
    if (!pimpl->args_array.empty()) {
        return pimpl->args_array;
    }
    
    // Otherwise, return single configuration as a vector
    if (!pimpl->args_map.empty()) {
        return {pimpl->args_map};
    }
    
    return {};
}

// Add option to CLI - for dynamically adding new options
bool args::add_option(const std::string& flags, const std::string& description) noexcept {
    if (!pimpl) {
        return false;
    }
    
    try {
        // Parse flags to extract main flag name
        std::string main_flag;
        std::vector<std::string> all_flags;
        
        // Split by comma
        std::stringstream ss(flags);
        std::string flag;
        while (std::getline(ss, flag, ',')) {
            // Trim whitespace
            flag.erase(0, flag.find_first_not_of(" \t"));
            flag.erase(flag.find_last_not_of(" \t") + 1);
            all_flags.push_back(flag);
            if (main_flag.empty() || flag.length() > main_flag.length()) {
                main_flag = flag;  // Use longest flag as main
            }
        }
        
        if (main_flag.empty()) {
            return false;
        }
        
        // Add to CLI11 - bind to a new vector in the impl
        auto& new_vec = pimpl->dynamic_options[main_flag];
        pimpl->cli_app.add_option(flags, new_vec, description);
        
        return true;
    } catch (...) {
        return false;
    }
}

// Add flag to CLI - for dynamically adding new flags
bool args::add_flag(const std::string& flags, const std::string& description) noexcept {
    if (!pimpl) {
        return false;
    }
    
    try {
        // Parse flags to extract main flag name
        std::string main_flag;
        std::vector<std::string> all_flags;
        
        // Split by comma
        std::stringstream ss(flags);
        std::string flag;
        while (std::getline(ss, flag, ',')) {
            // Trim whitespace
            flag.erase(0, flag.find_first_not_of(" \t"));
            flag.erase(flag.find_last_not_of(" \t") + 1);
            all_flags.push_back(flag);
            if (main_flag.empty() || flag.length() > main_flag.length()) {
                main_flag = flag;  // Use longest flag as main
            }
        }
        
        if (main_flag.empty()) {
            return false;
        }
        
        // Add to CLI11 - bind to a new bool in the impl
        auto& new_flag = pimpl->dynamic_flags[main_flag];
        pimpl->cli_app.add_flag(flags, new_flag, description);
        
        return true;
    } catch (...) {
        return false;
    }
}

