#include <MazeBuilder/args.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/writer.h>
#include <MazeBuilder/string_view_utils.h>

#include <string>
#include <regex>
#include <functional>
#include <sstream>
#include <iterator>
#include <algorithm>

// Include CLI11
#include <CLI11/CLI11.hpp>

using namespace mazes;

// Helper to improve JSON string detection
bool detect_json_string(const std::string& input) {
    // Clean input for easier checking
    std::string clean = string_view_utils::trim(input);
    
    // Check for backticks (JSON string indicator)
    if (clean.find('`') != std::string::npos) {
        return true;
    }
    
    // Check for direct JSON syntax (starts with { or [)
    if (!clean.empty()) {
        if (clean[0] == '{' || clean[0] == '[') {
            return true;
        }
    }
    
    return false;
}

// Add this helper function to parse the sliced array syntax at the top of the file, just after detect_json_string
bool parse_sliced_array(const std::string& value, std::unordered_map<std::string, std::string>& args_map) {
    std::regex slice_pattern(R"(\[(.*?):(.*?)\])");
    std::smatch matches;

    if (std::regex_match(value, matches, slice_pattern)) {
        std::string start_idx = matches[1].str();
        std::string end_idx = matches[2].str();

        // Store the parsed start and end indices for easier access
        if (!start_idx.empty()) {
            args_map["distances_start"] = start_idx;
        } else {
            args_map["distances_start"] = "0"; // Default to 0 if omitted
        }

        if (!end_idx.empty()) {
            args_map["distances_end"] = end_idx;
        } else {
            args_map["distances_end"] = "-1"; // -1 indicates the last cell
        }
        return true;
    }
    return false;
}

// Implementation class
class args::impl {
public:
    impl() : cli_app("MazeBuilder") {
        setup_cli();
    }

    CLI::App cli_app;
    std::unordered_map<std::string, std::string> args_map;
    std::vector<std::unordered_map<std::string, std::string>> args_array;
    void setup_cli() {
        // Configure CLI11 app
        cli_app.allow_extras(true);
        cli_app.prefix_command(false);
        cli_app.ignore_case(true);
        cli_app.option_defaults()->configurable(false);
        
        // Disable automatic help/version flags so we can capture them ourselves
        cli_app.set_help_flag();  // Remove default help flag
        cli_app.set_version_flag(); // Remove default version flag

        // Add common options with callbacks to store values in args_map
        cli_app.add_option("-j,--json", [this](const std::vector<std::string>& values) {
            if (!values.empty()) {
                args_map["-j"] = values[0];
                args_map["--json"] = values[0];
                args_map["json"] = values[0];
            }
            return true;
        }, "Parse JSON input file or string");
        
        cli_app.add_option("-o,--output", [this](const std::vector<std::string>& values) {
            if (!values.empty()) {
                args_map["-o"] = values[0];
                args_map["--output"] = values[0];
                args_map["output"] = values[0];
            }
            return true;
        }, "Output file");
        
        // Common flags with callbacks
        cli_app.add_flag("-h,--help", [this](size_t count) {
            if (count > 0) {
                args_map["-h"] = "true";
                args_map["--help"] = "true";
                args_map["help"] = "true";
            }
            return true;
        }, "Show help information")->take_first();
        
        cli_app.add_flag("-v,--version", [this](size_t count) {
            if (count > 0) {
                args_map["-v"] = "true";
                args_map["--version"] = "true";
                args_map["version"] = "true";
            }
            return true;
        }, "Show version information")->take_first();
        
        // Modified to handle sliced array syntax
        cli_app.add_option("-d,--distances", [this](const std::vector<std::string>& values) {
            if (!values.empty()) {
                std::string value = values[0];
                // Check if it's a sliced array format: [start:end], [start:], [:end]
                std::regex slice_pattern(R"(\[(.*?):(.*?)\])");
                std::smatch matches;
                
                if (std::regex_match(value, matches, slice_pattern)) {
                    // We have a sliced array format
                    std::string start_idx = matches[1].str();
                    std::string end_idx = matches[2].str();
                    
                    // Store the original format
                    args_map["-d"] = value;
                    args_map["--distances"] = value;
                    args_map["distances"] = value;
                    
                    // Store the parsed start and end indices for easier access
                    if (!start_idx.empty()) {
                        args_map["distances_start"] = start_idx;
                    } else {
                        args_map["distances_start"] = "0"; // Default to 0 if omitted
                    }
                    
                    if (!end_idx.empty()) {
                        args_map["distances_end"] = end_idx;
                    } else {
                        args_map["distances_end"] = "-1"; // -1 indicates the last cell
                    }
                } else {
                    // It's a regular value, store as is
                    args_map["-d"] = value;
                    args_map["--distances"] = value;
                    args_map["distances"] = value;
                }
            } else {
                // No value provided, treat as a flag
                args_map["-d"] = "true";
                args_map["--distances"] = "true";
                args_map["distances"] = "true";
            }
            return true;
        }, "Calculate distances between cells, optionally with a range [start:end]");
        
        // Common options with callbacks
        cli_app.add_option("-r,--rows", [this](const std::vector<std::string>& values) {
            if (!values.empty()) {
                args_map["-r"] = values[0];
                args_map["--rows"] = values[0];
                args_map["rows"] = values[0];
            }
            return true;
        }, "Number of rows in the maze");
        
        cli_app.add_option("-c,--columns", [this](const std::vector<std::string>& values) {
            if (!values.empty()) {
                args_map["-c"] = values[0];
                args_map["--columns"] = values[0];
                args_map["columns"] = values[0];
            }
            return true;
        }, "Number of columns in the maze");
        
        cli_app.add_option("-s,--seed", [this](const std::vector<std::string>& values) {
            if (!values.empty()) {
                args_map["-s"] = values[0];
                args_map["--seed"] = values[0];
                args_map["seed"] = values[0];
            }
            return true;
        }, "Random seed for maze generation");
        
        cli_app.add_option("--algo", [this](const std::vector<std::string>& values) {
            if (!values.empty()) {
                args_map["--algo"] = values[0];
                args_map["algo"] = values[0];
            }
            return true;
        }, "Algorithm to use for maze generation");
    }
};

// Constructor/Destructor implementation
args::args() noexcept : pimpl(std::make_unique<impl>()) {}
args::~args() noexcept = default;

// Copy constructor
args::args(const args& other) noexcept : 
    pimpl(std::make_unique<impl>()),
    args_map(other.args_map),
    args_array(other.args_array) {}

// Move constructor
args::args(args&& other) noexcept :
    pimpl(std::move(other.pimpl)),
    args_map(std::move(other.args_map)),
    args_array(std::move(other.args_array)) {}

// Copy assignment operator
args& args::operator=(const args& other) noexcept {
    if (this != &other) {
        pimpl = std::make_unique<impl>();
        args_map = other.args_map;
        args_array = other.args_array;
    }
    return *this;
}

// Move assignment operator
args& args::operator=(args&& other) noexcept {
    if (this != &other) {
        pimpl = std::move(other.pimpl);
        args_map = std::move(other.args_map);
        args_array = std::move(other.args_array);
    }
    return *this;
}

bool args::parse(const std::vector<std::string>& arguments) noexcept {
    try {
        // Clear existing data
        pimpl->args_map.clear();
        args_map.clear();
        args_array.clear();
        
        // Special handling for help and version flags
        for (const auto& arg : arguments) {
            if (arg == "-h" || arg == "--help") {
                args_map["-h"] = "true";
                args_map["--help"] = "true";
                args_map["help"] = "true";
                pimpl->args_map["-h"] = "true";
                pimpl->args_map["--help"] = "true";
                pimpl->args_map["help"] = "true";
            } else if (arg == "-v" || arg == "--version") {
                args_map["-v"] = "true";
                args_map["--version"] = "true";
                args_map["version"] = "true";
                pimpl->args_map["-v"] = "true";
                pimpl->args_map["--version"] = "true";
                pimpl->args_map["version"] = "true";
            }
        }
        
        // If help or version is set, just return true
        if (args_map.find("-h") != args_map.end() || args_map.find("-v") != args_map.end()) {
            return true;
        }
        
        // Direct parsing of argument pairs for short and long options
        for (size_t i = 0; i < arguments.size(); ++i) {
            std::string arg = arguments[i];
            if (arg.empty()) continue;
            
            // Process options with values
            if (arg[0] == '-') {
                bool isLongOpt = (arg.size() > 1 && arg[1] == '-');
                std::string key = arg;
                std::string value;
                
                // Check for --option=value format
                size_t equalPos = arg.find('=');
                if (equalPos != std::string::npos) {
                    key = arg.substr(0, equalPos);
                    value = arg.substr(equalPos + 1);
                    // Store with both forms
                    args_map[key] = value;
                    pimpl->args_map[key] = value;
                    
                    // Store without dashes too
                    if (isLongOpt) {
                        std::string cleanKey = key.substr(2);
                        args_map[cleanKey] = value;
                        pimpl->args_map[cleanKey] = value;
                    } else {
                        std::string cleanKey = key.substr(1);
                        args_map[cleanKey] = value;
                        pimpl->args_map[cleanKey] = value;
                    }
                    
                    // Special handling for distances with array slice notation
                    if (key == "-d" || key == "--distances") {
                        if (parse_sliced_array(value, args_map)) {
                            // Also update pimpl's map
                            pimpl->args_map["distances_start"] = args_map["distances_start"];
                            pimpl->args_map["distances_end"] = args_map["distances_end"];
                        }
                    }
                    
                    // Handle cross-mapping between short and long form
                    if (isLongOpt) {
                        std::string cleanKey = key.substr(2);
                        // Map common long options to their short form
                        if (cleanKey == "rows") {
                            args_map["-r"] = value;
                            pimpl->args_map["-r"] = value;
                            args_map["r"] = value;
                            pimpl->args_map["r"] = value;
                        } else if (cleanKey == "columns") {
                            args_map["-c"] = value;
                            pimpl->args_map["-c"] = value;
                            args_map["c"] = value;
                            pimpl->args_map["c"] = value;
                        } else if (cleanKey == "seed") {
                            args_map["-s"] = value;
                            pimpl->args_map["-s"] = value;
                            args_map["s"] = value;
                            pimpl->args_map["s"] = value;
                        } else if (cleanKey == "distances") {
                            args_map["-d"] = value;
                            pimpl->args_map["-d"] = value;
                            args_map["d"] = value;
                            pimpl->args_map["d"] = value;
                        } else if (cleanKey == "output") {
                            args_map["-o"] = value;
                            pimpl->args_map["-o"] = value;
                            args_map["o"] = value;
                            pimpl->args_map["o"] = value;
                        } else if (cleanKey == "json") {
                            args_map["-j"] = value;
                            pimpl->args_map["-j"] = value;
                            args_map["j"] = value;
                            pimpl->args_map["j"] = value;
                        }
                    }
                }
                // Check for -o value or --option value format
                else if (i + 1 < arguments.size() && arguments[i + 1][0] != '-') {
                    value = arguments[i + 1];
                    // Store with both forms
                    args_map[key] = value;
                    pimpl->args_map[key] = value;
                    
                    // Store without dashes too
                    if (isLongOpt) {
                        std::string cleanKey = key.substr(2);
                        args_map[cleanKey] = value;
                        pimpl->args_map[cleanKey] = value;
                    } else {
                        std::string cleanKey = key.substr(1);
                        args_map[cleanKey] = value;
                        pimpl->args_map[cleanKey] = value;
                        
                        // For common short options, also set the long form
                        if (cleanKey == "r") {
                            args_map["--rows"] = value;
                            pimpl->args_map["--rows"] = value;
                            args_map["rows"] = value;
                            pimpl->args_map["rows"] = value;
                        } else if (cleanKey == "c") {
                            args_map["--columns"] = value;
                            pimpl->args_map["--columns"] = value;
                            args_map["columns"] = value;
                            pimpl->args_map["columns"] = value;
                        } else if (cleanKey == "s") {
                            args_map["--seed"] = value;
                            pimpl->args_map["--seed"] = value;
                            args_map["seed"] = value;
                            pimpl->args_map["seed"] = value;
                        } else if (cleanKey == "o") {
                            args_map["--output"] = value;
                            pimpl->args_map["--output"] = value;
                            args_map["output"] = value;
                            pimpl->args_map["output"] = value;
                        } else if (cleanKey == "j") {
                            args_map["--json"] = value;
                            pimpl->args_map["--json"] = value;
                            args_map["json"] = value;
                            pimpl->args_map["json"] = value;
                        } else if (cleanKey == "d") {
                            args_map["--distances"] = value;
                            pimpl->args_map["--distances"] = value;
                            args_map["distances"] = value;
                            pimpl->args_map["distances"] = value;
                        }
                    }
                    
                    // Special handling for distances with array slice notation
                    if (key == "-d" || key == "--distances") {
                        if (parse_sliced_array(value, args_map)) {
                            // Also update pimpl's map
                            pimpl->args_map["distances_start"] = args_map["distances_start"];
                            pimpl->args_map["distances_end"] = args_map["distances_end"];
                        }
                    }
                    
                    // Skip the value in the next iteration
                    ++i;
                }
                // Flags without values
                else {
                    // Store with both forms
                    args_map[key] = "true";
                    pimpl->args_map[key] = "true";
                    
                    // Store without dashes too
                    if (isLongOpt) {
                        std::string cleanKey = key.substr(2);
                        args_map[cleanKey] = "true";
                        pimpl->args_map[cleanKey] = "true";
                    } else {
                        std::string cleanKey = key.substr(1);
                        args_map[cleanKey] = "true";
                        pimpl->args_map[cleanKey] = "true";
                    }
                }
            }
        }
        
        // Process JSON if present
        if (args_map.find("-j") != args_map.end() || args_map.find("--json") != args_map.end()) {
            std::string json_input;
            std::string key;
            
            if (args_map.find("-j") != args_map.end()) {
                json_input = args_map["-j"];
                key = "-j";
            } else {
                json_input = args_map["--json"];
                key = "--json";
            }
            
            json_input = string_view_utils::trim(json_input);
            
            // Use our helper to detect if this is a string input
            bool is_string_input = detect_json_string(json_input);
            
            // Clean up input if it's a string
            if (is_string_input) {
                json_input.erase(std::remove(json_input.begin(), json_input.end(), '`'), json_input.end());
                json_input = string_view_utils::trim(json_input);
            }
            
            // If we're processing a filename (not JSON string) and no output file has been specified,
            // automatically generate one based on the input filename
            if (!is_string_input && 
                args_map.find("-o") == args_map.end() && 
                args_map.find("--output") == args_map.end()) {
                std::string output_name = generate_output_filename(json_input, false);
                args_map["-o"] = output_name;
                args_map["--output"] = output_name;
                args_map["output"] = output_name;
                pimpl->args_map["-o"] = output_name;
                pimpl->args_map["--output"] = output_name;
                pimpl->args_map["output"] = output_name;
            }
            
            return process_json_input(json_input, is_string_input, key);
        }
        
        return true;
    } catch (const CLI::ParseError&) {
        // Still consider help and version requests as successful parses
        if (args_map.find("-h") != args_map.end()) {
            return true;
        } else if (args_map.find("-v") != args_map.end()) {
            return true;
        }
        return false;
    } catch (...) {
        return false;
    }
}

bool args::parse(const std::string& arguments) noexcept {
    // Check if this is a JSON string directly
    std::string trimmed = string_view_utils::trim(arguments);
    if (trimmed.find("-j `{") == 0 || trimmed.find("-j `[") == 0 || 
        trimmed.find("--json `{") == 0 || trimmed.find("--json `[") == 0 ||
        trimmed.find("--json=`{") == 0 || trimmed.find("--json=`[") == 0) {
        
        // Extract the key and JSON content
        std::string key;
        std::string json_content;
        
        if (trimmed.find("-j") == 0) {
            key = "-j";
            json_content = trimmed.substr(3); // Skip "-j "
        } else if (trimmed.find("--json=") == 0) {
            key = "--json";
            json_content = trimmed.substr(7); // Skip "--json="
        } else {
            key = "--json";
            json_content = trimmed.substr(7); // Skip "--json "
        }
        
        // Clean up backticks
        json_content.erase(std::remove(json_content.begin(), json_content.end(), '`'), json_content.end());
        json_content = string_view_utils::trim(json_content);
        
        // Set the key in args_map
        args_map.clear();
        pimpl->args_map.clear();
        args_map[key] = json_content;
        if (key == "-j") {
            args_map["--json"] = json_content;
            args_map["json"] = json_content;
        } else {
            args_map["-j"] = json_content;
            args_map["json"] = json_content;
        }
        
        // Process as JSON string
        return process_json_input(json_content, true, key);
    }
    
    // Regular command-line arguments
    std::istringstream iss(arguments);
    std::vector<std::string> args_vec((std::istream_iterator<std::string>(iss)),
        std::istream_iterator<std::string>());
    return parse(args_vec);
}

bool args::parse(int argc, char** argv) noexcept {
    try {
        // Clear existing data
        pimpl->args_map.clear();
        args_map.clear();
        args_array.clear();
        
        // Special handling for help and version flags
        for (int i = 1; i < argc; ++i) {
            std::string arg(argv[i]);
            if (arg == "-h" || arg == "--help") {
                args_map["-h"] = "true";
                args_map["--help"] = "true";
                args_map["help"] = "true";
                pimpl->args_map["-h"] = "true";
                pimpl->args_map["--help"] = "true";
                pimpl->args_map["help"] = "true";
            } else if (arg == "-v" || arg == "--version") {
                args_map["-v"] = "true";
                args_map["--version"] = "true";
                args_map["version"] = "true";
                pimpl->args_map["-v"] = "true";
                pimpl->args_map["--version"] = "true";
                pimpl->args_map["version"] = "true";
            }
        }
        
        // If help or version is set, just return true
        if (args_map.find("-h") != args_map.end() || args_map.find("-v") != args_map.end()) {
            return true;
        }
        
        // Convert to vector of strings for consistency with the other parse method
        std::vector<std::string> args_vec;
        for (int i = 1; i < argc; ++i) {
            args_vec.push_back(std::string(argv[i]));
        }
        
        return parse(args_vec);
    } catch (...) {
        return false;
    }
}

bool args::process_json_input(const std::string& json_input, bool is_string_input, const std::string& key) noexcept {
    try {
        json_helper jh{};
        bool result = false;
        std::string refined_json = json_input;
        
        // Check if input has JSON structure markers (for string-based input)
        bool looks_like_json = (refined_json.find('{') != std::string::npos) || 
                              (refined_json.find('[') != std::string::npos);
        
        // Force string input if it looks like JSON
        if (looks_like_json) {
            is_string_input = true;
        }

        // Handle string input (JSON string) vs file input
        if (is_string_input) {
            // Try to parse as an array first
            if (jh.from_array(refined_json, args_array)) {
                // Successfully parsed as array
                result = true;
                // Also populate args_map with first item for easy access
                if (!args_array.empty()) {
                    for (const auto& kv : args_array[0]) {
                        args_map[kv.first] = kv.second;
                        pimpl->args_map[kv.first] = kv.second;
                        
                        // Also add with dashed forms for consistency
                        args_map["-" + kv.first] = kv.second;
                        args_map["--" + kv.first] = kv.second;
                        pimpl->args_map["-" + kv.first] = kv.second;
                        pimpl->args_map["--" + kv.first] = kv.second;
                    }
                }
            } else {
                // Try standard object parsing
                std::unordered_map<std::string, std::string> temp_map;
                result = jh.from(refined_json, temp_map);
                
                if (result) {
                    // Copy to args_map and add dashed versions
                    for (const auto& kv : temp_map) {
                        args_map[kv.first] = kv.second;
                        pimpl->args_map[kv.first] = kv.second;
                        
                        if (kv.first[0] != '-') {
                            // Add dashed versions if original doesn't have them
                            args_map["-" + kv.first] = kv.second;
                            args_map["--" + kv.first] = kv.second;
                            pimpl->args_map["-" + kv.first] = kv.second;
                            pimpl->args_map["--" + kv.first] = kv.second;
                        }
                    }
                }
            }
        } else {
            // Handle file input - try loading as array first
            if (jh.load_array(refined_json, args_array)) {
                result = true;
                // Also populate args_map with first item for easy access
                if (!args_array.empty()) {
                    for (const auto& kv : args_array[0]) {
                        args_map[kv.first] = kv.second;
                        pimpl->args_map[kv.first] = kv.second;
                        
                        // Also add with dashed forms for consistency
                        args_map["-" + kv.first] = kv.second;
                        args_map["--" + kv.first] = kv.second;
                        pimpl->args_map["-" + kv.first] = kv.second;
                        pimpl->args_map["--" + kv.first] = kv.second;
                    }
                }
            } else {
                // Try standard object loading
                std::unordered_map<std::string, std::string> temp_map;
                result = jh.load(refined_json, temp_map);
                
                if (result) {
                    // Copy to args_map and add dashed versions
                    for (const auto& kv : temp_map) {
                        args_map[kv.first] = kv.second;
                        pimpl->args_map[kv.first] = kv.second;
                        
                        if (kv.first[0] != '-') {
                            // Add dashed versions if original doesn't have them
                            args_map["-" + kv.first] = kv.second;
                            args_map["--" + kv.first] = kv.second;
                            pimpl->args_map["-" + kv.first] = kv.second;
                            pimpl->args_map["--" + kv.first] = kv.second;
                        }
                    }
                }
            }
        }
        
        // After populating args_map from the object/array, check for sliced array notation in distances field
        if (args_map.find("distances") != args_map.end()) {
            std::string distances_value = args_map["distances"];
            
            // If it's a JSON string, it might have quotes around it
            if (distances_value.size() >= 2 && distances_value.front() == '"' && distances_value.back() == '"') {
                distances_value = distances_value.substr(1, distances_value.length() - 2);
                // Update the value without quotes
                args_map["distances"] = distances_value;
                pimpl->args_map["distances"] = distances_value;
            }
            
            // Check if it's a sliced array format
            if (parse_sliced_array(distances_value, args_map)) {
                // Also update pimpl's map
                pimpl->args_map["distances_start"] = args_map["distances_start"];
                pimpl->args_map["distances_end"] = args_map["distances_end"];
            }
        }
        
        // If user specified an output file, make sure all variants are set
        if (args_map.find("-o") != args_map.end()) {
            std::string output_value = args_map["-o"];
            args_map["--output"] = output_value;
            args_map["output"] = output_value;
            pimpl->args_map["--output"] = output_value;
            pimpl->args_map["output"] = output_value;
        } else if (args_map.find("--output") != args_map.end()) {
            std::string output_value = args_map["--output"];
            args_map["-o"] = output_value;
            args_map["output"] = output_value;
            pimpl->args_map["-o"] = output_value;
            pimpl->args_map["output"] = output_value;
        } else if (args_map.find("output") != args_map.end()) {
            std::string output_value = args_map["output"];
            args_map["-o"] = output_value;
            args_map["--output"] = output_value;
            pimpl->args_map["-o"] = output_value;
            pimpl->args_map["--output"] = output_value;
        }
        // Auto-generate output filename if not already specified
        else if (result) {
            std::string output_name = generate_output_filename(json_input, is_string_input);
            args_map["output"] = output_name;
            args_map["-o"] = output_name;
            args_map["--output"] = output_name;
            pimpl->args_map["output"] = output_name; 
            pimpl->args_map["-o"] = output_name;
            pimpl->args_map["--output"] = output_name;
        }
        
        // Store the json input source in the args_map
        if (result) {
            args_map[key] = json_input;
            pimpl->args_map[key] = json_input;
            if (key == "-j") {
                args_map["--json"] = json_input;
                args_map["json"] = json_input;
                pimpl->args_map["--json"] = json_input;
                pimpl->args_map["json"] = json_input;
            } else if (key == "--json") {
                args_map["-j"] = json_input;
                args_map["json"] = json_input;
                pimpl->args_map["-j"] = json_input;
                pimpl->args_map["json"] = json_input;
            }
        }
        
        return result;
    } catch (...) {
        return false;
    }
}

void args::clear() noexcept {
    args_map.clear();
    args_array.clear();
    pimpl->args_map.clear();
    pimpl->args_array.clear();
}

std::optional<std::string> args::get(const std::string& key) const noexcept {
    auto it = args_map.find(key);
    if (it != args_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

const std::unordered_map<std::string, std::string>& args::get() const noexcept {
    return args_map;
}

const std::vector<std::unordered_map<std::string, std::string>>& args::get_array() const noexcept {
    return args_array;
}

bool args::has_array() const noexcept {
    return !args_array.empty();
}

void args::set(const std::string& key, const std::string& value) noexcept {
    args_map[key] = value;
    pimpl->args_map[key] = value;
}

std::string args::generate_output_filename(const std::string& input_value, bool is_string_input) const noexcept {
    // For string input, use a default name
    if (is_string_input || input_value.empty() || 
        input_value.find('{') != std::string::npos || 
        input_value.find('[') != std::string::npos) {
        return "output.json";
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

bool args::add_option(const std::string& option_name, const std::string& description) noexcept {
    try {
        // Parse the option name to separate short/long options
        std::vector<std::string> option_names = CLI::detail::split_names(option_name);
        
        // Add the option to CLI11 app with lambda capture for both args_map and pimpl->args_map
        auto option = pimpl->cli_app.add_option(option_name, [this, option_names](const std::vector<std::string>& values) {
            if (!values.empty()) {
                // Extract the first value from the results
                std::string value = values[0];
                
                // Store in the map with all possible forms (with and without dashes)
                for (const auto& opt_name : option_names) {
                    // Handle both long and short option formats
                    if (opt_name.size() > 1) {
                        // Long option (--option)
                        std::string with_dashes = "--" + opt_name;
                        this->args_map[with_dashes] = value;
                        this->pimpl->args_map[with_dashes] = value;
                        this->args_map[opt_name] = value;
                        this->pimpl->args_map[opt_name] = value;
                    } else {
                        // Short option (-o)
                        std::string with_dash = "-" + opt_name;
                        this->args_map[with_dash] = value;
                        this->pimpl->args_map[with_dash] = value;
                        this->args_map[opt_name] = value;
                        this->pimpl->args_map[opt_name] = value;
                    }
                }
            }
            return true;
        }, description);
        
        return option != nullptr;
    } catch (...) {
        // In case of any exceptions, just return false
        return false;
    }
}

bool args::add_flag(const std::string& flag_name, const std::string& description) noexcept {
    try {
        // Parse the flag name to separate short/long options
        std::vector<std::string> flag_names = CLI::detail::split_names(flag_name);
        
        // Add the flag to CLI11 app with lambda capture for both args_map and pimpl->args_map
        auto flag = pimpl->cli_app.add_flag(flag_name, [this, flag_names](size_t count) {
            if (count > 0) {
                // Store in the map with all possible forms (with and without dashes)
                for (const auto& opt_name : flag_names) {
                    // Handle both long and short flag formats
                    if (opt_name.size() > 1) {
                        // Long flag (--flag)
                        std::string with_dashes = "--" + opt_name;
                        this->args_map[with_dashes] = "true";
                        this->pimpl->args_map[with_dashes] = "true";
                        this->args_map[opt_name] = "true";
                        this->pimpl->args_map[opt_name] = "true";
                    } else {
                        // Short flag (-f)
                        std::string with_dash = "-" + opt_name;
                        this->args_map[with_dash] = "true";
                        this->pimpl->args_map[with_dash] = "true";
                        this->args_map[opt_name] = "true";
                        this->pimpl->args_map[opt_name] = "true";
                    }
                }
            }
            return true;
        }, description);
        
        return flag != nullptr;
    } catch (...) {
        // In case of any exceptions, just return false
        return false;
    }
}

