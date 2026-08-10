#include <MazeBuilder/args.h>
#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/string_utils.h>

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <ranges>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace mazes;

// ============================================================================
// Token and Parser Implementation
// ============================================================================
namespace
{
    enum class TokenType
    {
        PROGRAM, // program name
        SHORT_FLAG, // -r, -c, etc.
        LONG_OPTION, // --rows, --columns, etc.
        VALUE, // any value
        SLICE_NOTATION, // [0:10]
        END_OF_INPUT
    };

    struct Token
    {
        TokenType type;
        std::string text;
    };

    // Tokenizer: converts input string vector into token stream
    class Tokenizer
    {
    public:
        explicit Tokenizer(const std::vector<std::string>& args, const bool skip_first = false)
            : args_(args), pos_(skip_first ? 1 : 0), has_program_name_(skip_first)
        {
        }

        Token next()
        {
            if (pos_ >= args_.size())
            {
                return Token { .type = TokenType::END_OF_INPUT, .text = ""};
            }

            const auto& arg = args_[pos_++];

            // Check for slice notation: must be well-formed [..:..]
            // Properly formatted slices have '[' at start, ']' at end, and exactly one ':' in between
            // Minimum valid slice is [:]  (size 3)
            if (arg.starts_with('[') && arg.ends_with(']') && arg.size() >= 3)
            {
                auto first_colon = arg.find(':');
                if (first_colon != std::string::npos && first_colon > 0 && first_colon < arg.size() - 1)
                {
                    // Check if there's only one colon
                    if (arg.find(':', first_colon + 1) == std::string::npos)
                    {
                        return Token { .type = TokenType::SLICE_NOTATION, .text = arg };
                    }
                }
            }

            // Check for long option
            if (arg.starts_with("--"))
            {
                return Token {.type = TokenType::LONG_OPTION, .text = arg};
            }

            // Check for short flag: must start with - (not --) and have at least 2 characters
            // The second character must be a letter (the flag character)
            // Everything after can be a concatenated value (e.g., -r10, -asidewinder)
            if (arg.starts_with("-") && arg.size() >= 2 && !arg.starts_with("--"))
            {
                if (std::isalpha(static_cast<unsigned char>(arg[1])))
                {
                    return Token {.type = TokenType::SHORT_FLAG, .text = arg};
                }
                // Invalid: single dash with non-letter second char (e.g., "-")
                return Token {.type = TokenType::VALUE, .text = arg};
            }

            // Otherwise it's a value (or program name if first and skip_first was true)
            if (pos_ == 1 && has_program_name_ && !arg.starts_with("-"))
            {
                return Token {.type = TokenType::PROGRAM, .text = arg};
            }

            return Token {.type = TokenType::VALUE, .text = arg};
        }

        [[nodiscard]] bool has_more() const { return pos_ < args_.size(); }

        void unread() noexcept
        {
            if (pos_ > 0)
            {
                --pos_;
            }
        }
    private:
        const std::vector<std::string>& args_;
        size_t pos_;
        bool has_program_name_;
    };

    // Parser: processes tokens and builds argument map
    class Parser
    {
    public:
        using ArgMap = std::unordered_map<std::string, std::string>;

        Parser() = default;

        bool parse(const std::vector<std::string>& args, bool has_program_name)
        {
            Tokenizer tokenizer(args, has_program_name);
            current_map_.clear();

            try
            {
                // Handle program name if present
                if (has_program_name && !args.empty())
                {
                    store_value(args::APP_KEY, args[0]);
                }

                while (tokenizer.has_more())
                {
                    auto token = tokenizer.next();

                    if (token.type == TokenType::END_OF_INPUT)
                    {
                        break;
                    }

                    if (token.type == TokenType::PROGRAM)
                    {
                        store_value(args::APP_KEY, token.text);
                        continue;
                    }

                    if (token.type == TokenType::SHORT_FLAG)
                    {
                        if (!parse_short_flag(token, tokenizer))
                        {
                            return false;
                        }
                    }
                    else if (token.type == TokenType::LONG_OPTION)
                    {
                        if (!parse_long_option(token, tokenizer))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        // Unexpected value
#ifdef MAZE_DEBUG
                        global_async_logger().log("Unexpected value: {}", token.text);
#endif
                        return false;
                    }
                }

                // Post-processing: if we parsed JSON, delegate to JSON handler
                if (auto it = current_map_.find(args::JSON_WORD_STR); it != current_map_.end())
                {
                    bool json_success = process_json(it->second);

                    // For JSON strings, process_json merges into current_map
                    // For JSON files (arrays), process_json populates results directly
                    // Check if results was populated by process_json
                    if (!results_.empty())
                    {
                        return json_success; // JSON file (array) case
                    }

                    // JSON string case: always move current_map to results (even on failure)
                    // This preserves the state for error reporting
                    results_.clear();
                    results_.push_back(std::move(current_map_));
                    return json_success;
                }

                // Success: move current_map to results (even if empty - program name only is valid)
                results_.clear();
                results_.push_back(std::move(current_map_));
                return true;
            }
            catch (const std::exception& e)
            {
                global_async_logger().log("Parse error: {}", e.what());
                return false;
            }
        }

        [[nodiscard]] const std::vector<ArgMap>& results() const { return results_; }

    private:
        ArgMap current_map_;
        std::vector<ArgMap> results_;

        // Store value with automatic aliasing (flag/option/word forms)
        void store_value(std::string_view key, std::string_view value)
        {
            // Map of word-form keys to their flag and option aliases
            static const std::unordered_map<std::string_view, std::tuple<
                                                std::string_view, std::string_view, std::string_view>> aliases = {
                {args::ROW_WORD_STR, {args::ROW_FLAG_STR, args::ROW_OPTION_STR, args::ROW_WORD_STR}},
                {args::COLUMN_WORD_STR, {args::COLUMN_FLAG_STR, args::COLUMN_OPTION_STR, args::COLUMN_WORD_STR}},
                {args::LEVEL_WORD_STR, {args::LEVEL_FLAG_STR, args::LEVEL_OPTION_STR, args::LEVEL_WORD_STR}},
                {args::SEED_WORD_STR, {args::SEED_FLAG_STR, args::SEED_OPTION_STR, args::SEED_WORD_STR}},
                {args::ALGO_ID_WORD_STR, {args::ALGO_ID_FLAG_STR, args::ALGO_ID_OPTION_STR, args::ALGO_ID_WORD_STR}},
                {
                    args::OUTPUT_ID_WORD_STR,
                    {args::OUTPUT_ID_FLAG_STR, args::OUTPUT_ID_OPTION_STR, args::OUTPUT_ID_WORD_STR}
                },
                {args::JSON_WORD_STR, {args::JSON_FLAG_STR, args::JSON_OPTION_STR, args::JSON_WORD_STR}},
                {
                    args::DISTANCES_WORD_STR,
                    {args::DISTANCES_FLAG_STR, args::DISTANCES_OPTION_STR, args::DISTANCES_WORD_STR}
                },
                {args::MASK_WORD_STR, {args::MASK_FLAG_STR, args::MASK_OPTION_STR, args::MASK_WORD_STR}},
                {
                    args::IMAGE_WIDTH_WORD_STR,
                    {args::IMAGE_WIDTH_FLAG_STR, args::IMAGE_WIDTH_OPTION_STR, args::IMAGE_WIDTH_WORD_STR}
                },
                {
                    args::IMAGE_HEIGHT_WORD_STR,
                    {args::IMAGE_HEIGHT_FLAG_STR, args::IMAGE_HEIGHT_OPTION_STR, args::IMAGE_HEIGHT_WORD_STR}
                },
                {args::HELP_WORD_STR, {args::HELP_FLAG_STR, args::HELP_OPTION_STR, args::HELP_WORD_STR}},
                {args::VERSION_WORD_STR, {args::VERSION_FLAG_STR, args::VERSION_OPTION_STR, args::VERSION_WORD_STR}}
            };

            std::string_view word_key = key;

            // Find the alias tuple for this key
            auto it = aliases.find(word_key);
            if (it != aliases.end())
            {
                const auto& [flag, option, word] = it->second;
                if (!flag.empty()) current_map_[std::string(flag)] = value;
                if (!option.empty()) current_map_[std::string(option)] = value;
                current_map_[std::string(word)] = value;
            }
            else
            {
                // No alias, just store as-is
                current_map_[std::string(key)] = value;
            }
        }

        // Normalize flag/option to word-form key
        static std::string_view normalize_key(std::string_view key)
        {
            static const std::unordered_map<std::string_view, std::string_view> normalization = {
                {args::ROW_FLAG_STR, args::ROW_WORD_STR}, {args::ROW_OPTION_STR, args::ROW_WORD_STR},
                {args::COLUMN_FLAG_STR, args::COLUMN_WORD_STR}, {args::COLUMN_OPTION_STR, args::COLUMN_WORD_STR},
                {args::LEVEL_FLAG_STR, args::LEVEL_WORD_STR}, {args::LEVEL_OPTION_STR, args::LEVEL_WORD_STR},
                {args::SEED_FLAG_STR, args::SEED_WORD_STR}, {args::SEED_OPTION_STR, args::SEED_WORD_STR},
                {args::ALGO_ID_FLAG_STR, args::ALGO_ID_WORD_STR}, {args::ALGO_ID_OPTION_STR, args::ALGO_ID_WORD_STR},
                {args::OUTPUT_ID_FLAG_STR, args::OUTPUT_ID_WORD_STR},
                {args::OUTPUT_ID_OPTION_STR, args::OUTPUT_ID_WORD_STR},
                {args::JSON_FLAG_STR, args::JSON_WORD_STR}, {args::JSON_OPTION_STR, args::JSON_WORD_STR},
                {args::DISTANCES_FLAG_STR, args::DISTANCES_WORD_STR},
                {args::DISTANCES_OPTION_STR, args::DISTANCES_WORD_STR},
                {args::MASK_FLAG_STR, args::MASK_WORD_STR}, {args::MASK_OPTION_STR, args::MASK_WORD_STR},
                {args::IMAGE_WIDTH_FLAG_STR, args::IMAGE_WIDTH_WORD_STR},
                {args::IMAGE_WIDTH_OPTION_STR, args::IMAGE_WIDTH_WORD_STR},
                {args::IMAGE_HEIGHT_FLAG_STR, args::IMAGE_HEIGHT_WORD_STR},
                {args::IMAGE_HEIGHT_OPTION_STR, args::IMAGE_HEIGHT_WORD_STR},
                {args::HELP_FLAG_STR, args::HELP_WORD_STR}, {args::HELP_OPTION_STR, args::HELP_WORD_STR},
                {args::VERSION_FLAG_STR, args::VERSION_WORD_STR}, {args::VERSION_OPTION_STR, args::VERSION_WORD_STR}
            };

            if (auto it = normalization.find(key); it != normalization.end())
            {
                return it->second;
            }
            return key;
        }

        // Parse short flag: -r, -h, -r10, -r 10, -d[0:10]
        bool parse_short_flag(const Token& token, Tokenizer& tokenizer)
        {
            std::string flag = token.text;

            // Check for concatenated value (e.g., -r10)
            if (flag.size() > 2)
            {
                std::string_view key(flag.data(), 2); // -r
                std::string_view value(flag.data() + 2, flag.size() - 2); // 10

                auto word_key = normalize_key(key);

                // Special case: -d[...] distances with slice
                if (word_key == args::DISTANCES_WORD_STR && value.starts_with('['))
                {
                    return parse_distances_value(word_key, std::string(value));
                }

                store_value(word_key, value);
                return true;
            }

            // Boolean flags (no value expected)
            auto word_key = normalize_key(flag);

            // Flag with value: check for next token
            if (!tokenizer.has_more())
            {
                // Distances flag can be standalone
                if (word_key == args::DISTANCES_WORD_STR)
                {
                    store_value(word_key, args::TRUE_VALUE);
                    return true;
                }
#ifdef MAZE_DEBUG
                global_async_logger().log("Flag {} requires a value", flag);
#endif
                return false;
            }

            auto value_token = tokenizer.next();

            // Handle slice notation for distances
            if (word_key == args::DISTANCES_WORD_STR)
            {
                if (value_token.type == TokenType::SLICE_NOTATION)
                {
                    return parse_distances_value(word_key, value_token.text);
                }
                else if (value_token.type == TokenType::VALUE)
                {
                    // Distances with non-slice value is invalid
#ifdef MAZE_DEBUG
                    global_async_logger().log("Distances flag requires slice notation, got: {}", value_token.text);
#endif
                    return false;
                }
                // Next token is another flag/option; treat distances as boolean and put token back.
                tokenizer.unread();
                store_value(word_key, args::TRUE_VALUE);
                return true;
            }

            // Regular value
            store_value(word_key, value_token.text);
            return true;
        }

        // Parse long option: --help, --rows=10, --rows 10
        bool parse_long_option(const Token& token, Tokenizer& tokenizer)
        {
            std::string option = token.text;

            // Check for embedded value (--rows=10)
            if (const auto eq_pos = option.find('='); eq_pos != std::string::npos)
            {
                const std::string_view key(option.data(), eq_pos);
                const std::string_view value(option.data() + eq_pos + 1, option.size() - eq_pos - 1);

                const auto word_key = normalize_key(key);

                // Special case: --distances=[...]
                if (word_key == args::DISTANCES_WORD_STR)
                {
                    if (value.starts_with('['))
                    {
                        return parse_distances_value(word_key, std::string(value));
                    }
                    else
                    {
                        // Distances with non-slice value is invalid
#ifdef MAZE_DEBUG
                        global_async_logger().log("Distances option requires slice notation, got: {}", value);
#endif
                        return false;
                    }
                }

                store_value(word_key, value);
                return true;
            }

            // Boolean options (no value expected)
            const auto word_key = normalize_key(option);
            if (word_key == args::HELP_WORD_STR || word_key == args::VERSION_WORD_STR)
            {
                store_value(word_key, args::TRUE_VALUE);
                return true;
            }

            // Option with value: check for next token
            if (!tokenizer.has_more())
            {
                // Distances can be standalone
                if (word_key == args::DISTANCES_WORD_STR)
                {
                    store_value(word_key, args::TRUE_VALUE);
                    return true;
                }
#ifdef MAZE_DEBUG
                global_async_logger().log("Option {} requires a value", option);
#endif
                return false;
            }

            auto [type, text] = tokenizer.next();

            // Handle slice notation for distances
            if (word_key == args::DISTANCES_WORD_STR)
            {
                if (type == TokenType::SLICE_NOTATION)
                {
                    return parse_distances_value(word_key, text);
                }
                if (type == TokenType::VALUE)
                {
                    // Distances with non-slice value is invalid
#ifdef MAZE_DEBUG
                    global_async_logger().log("Distances option requires slice notation, got: {}", text);
#endif
                    return false;
                }
                // Next token is another flag/option; treat distances as boolean and put token back.
                tokenizer.unread();
                store_value(word_key, args::TRUE_VALUE);
                return true;
            }

            // Regular value
            store_value(word_key, text);
            return true;
        }

        // Parse distances value with slice notation [start:end]
        bool parse_distances_value(const std::string_view key, const std::string& value)
        {
            static const std::regex slice_pattern(R"(\[(\d*):(-?\d*)\])");

            if (std::smatch matches; std::regex_match(value, matches, slice_pattern))
            {
                const auto start_str = matches[1].str();
                const auto end_str = matches[2].str();

                // Default values
                int start = configurator::DEFAULT_DISTANCES_START;
                int end = configurator::DEFAULT_DISTANCES_END;

                if (!start_str.empty())
                {
                    std::from_chars(start_str.data(), start_str.data() + start_str.size(), start);
                }
                if (!end_str.empty())
                {
                    std::from_chars(end_str.data(), end_str.data() + end_str.size(), end);
                }

                // Store slice notation and individual values
                const std::string normalized_slice = "[" + std::to_string(start) + ":" + std::to_string(end) + "]";
                store_value(key, normalized_slice);

                current_map_[args::DISTANCES_START_STR] = std::to_string(start);
                current_map_[args::DISTANCES_END_STR] = std::to_string(end);

                return true;
            }

#ifdef MAZE_DEBUG
            global_async_logger().log("Invalid slice notation: {}", value);
#endif
            return false;
        }

        // Process JSON input (file or string)
        bool process_json(const std::string& json_input)
        {
            try
            {
                // Strip whitespace and check format
                auto trimmed = string_utils::strip_whitespace(json_input);

                json_helper jh{};

                // JSON string (backtick-enclosed)
                if (trimmed.starts_with('`') && trimmed.ends_with('`'))
                {
                    auto clean_json = trimmed.substr(1, trimmed.size() - 2);

                    std::unordered_map<std::string, std::string> parsed_json;
                    if (!jh.from(clean_json, parsed_json))
                    {
                        return false;
                    }

                    // Merge JSON values into current map
                    for (const auto& [k, v] : parsed_json)
                    {
                        auto word_key = normalize_key(k);
                        store_value(word_key, v);
                    }

                    return true;
                }

                // JSON file
                std::filesystem::path resolved_path{json_input};

                if (!std::filesystem::exists(resolved_path))
                {
                    // Try fallback directories
                    static constexpr const char* fallback_dirs[] = {"tests", "build-msvc-tests/tests/RelWithDebInfo"};
                    bool found = false;

                    for (const auto& dir : fallback_dirs)
                    {
                        if (auto candidate = std::filesystem::path(dir) / json_input; std::filesystem::exists(candidate))
                        {
                            resolved_path = candidate;
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        throw std::runtime_error("File not found: " + json_input);
                    }
                }

                auto resolved_str = resolved_path.string();

                // Try loading as array first
                std::vector<std::unordered_map<std::string, std::string>> parsed_array;
                if (jh.load_array(resolved_str, parsed_array))
                {
                    results_.clear();

                    for (const auto& json_obj : parsed_array)
                    {
                        ArgMap map;
                        for (const auto& [k, v] : json_obj)
                        {
                            auto word_key = normalize_key(k);

                            // Use aliasing for known keys
                            static const std::unordered_map<
                                    std::string_view, std::tuple<std::string_view, std::string_view, std::string_view>>
                                aliases = {
                                    {
                                        args::ROW_WORD_STR,
                                        {args::ROW_FLAG_STR, args::ROW_OPTION_STR, args::ROW_WORD_STR}
                                    },
                                    {
                                        args::COLUMN_WORD_STR,
                                        {args::COLUMN_FLAG_STR, args::COLUMN_OPTION_STR, args::COLUMN_WORD_STR}
                                    },
                                    {
                                        args::LEVEL_WORD_STR,
                                        {args::LEVEL_FLAG_STR, args::LEVEL_OPTION_STR, args::LEVEL_WORD_STR}
                                    },
                                    {
                                        args::SEED_WORD_STR,
                                        {args::SEED_FLAG_STR, args::SEED_OPTION_STR, args::SEED_WORD_STR}
                                    },
                                    {
                                        args::ALGO_ID_WORD_STR,
                                        {args::ALGO_ID_FLAG_STR, args::ALGO_ID_OPTION_STR, args::ALGO_ID_WORD_STR}
                                    },
                                    {
                                        args::OUTPUT_ID_WORD_STR,
                                        {args::OUTPUT_ID_FLAG_STR, args::OUTPUT_ID_OPTION_STR, args::OUTPUT_ID_WORD_STR}
                                    },
                                    {
                                        args::DISTANCES_WORD_STR,
                                        {args::DISTANCES_FLAG_STR, args::DISTANCES_OPTION_STR, args::DISTANCES_WORD_STR}
                                    }
                                };

                            if (auto it = aliases.find(word_key); it != aliases.cend())
                            {
                                const auto& [flag, option, word] = it->second;
                                if (!flag.empty()) map[std::string(flag)] = v;
                                if (!option.empty()) map[std::string(option)] = v;
                                map[std::string(word)] = v;
                            }
                            else
                            {
                                map[std::string(k)] = v;
                            }
                        }

                        // Add JSON keys
                        map[args::JSON_FLAG_STR] = json_input;
                        map[args::JSON_OPTION_STR] = json_input;
                        map[args::JSON_WORD_STR] = json_input;

                        results_.push_back(std::move(map));
                    }

                    return true;
                }

                // Try loading as single object
                if (std::unordered_map<std::string, std::string> parsed_json; jh.load(resolved_str, parsed_json))
                {
                    for (const auto& [k, v] : parsed_json)
                    {
                        auto word_key = normalize_key(k);
                        store_value(word_key, v);
                    }
                    return true;
                }

                return false;
            }
            catch (const std::exception& e)
            {
                global_async_logger().log("JSON processing error: {}", e.what());
                return false;
            }
        }
    };
} // anonymous namespace

// ============================================================================
// args::impl (PIMPL)
// ============================================================================

class args::impl
{
public:
    impl() = default;

    std::vector<std::unordered_map<std::string, std::string>> arguments;

    bool parse(const std::vector<std::string>& args_vec, const bool has_program_name)
    {
        Parser parser;
        const bool success = parser.parse(args_vec, has_program_name);

        // Always update arguments if we have results, even on partial failure
        // This preserves the state up to the point of failure
        if (!parser.results().empty())
        {
            arguments = parser.results();
        }

        return success;
    }

    void clear() noexcept
    {
        arguments.clear();
    }
};

// ============================================================================
// args public API
// ============================================================================

args::args() noexcept : pimpl{std::make_unique<impl>()}
{
}

args::~args() = default;

args::args(args&& other) noexcept = default;

args& args::operator=(args&& other) noexcept = default;

args::args(const args& other) : pimpl{std::make_unique<impl>()}
{
    if (other.pimpl)
    {
        pimpl->arguments = other.pimpl->arguments;
    }
}

args& args::operator=(const args& other)
{
    if (this != &other && other.pimpl)
    {
        pimpl = std::make_unique<impl>();
        pimpl->arguments = other.pimpl->arguments;
    }
    return *this;
}

bool args::parse(const std::vector<std::string>& arguments, const bool has_program_name_as_first_arg) const noexcept
{
    if (arguments.empty())
    {
        return false;
    }

    try
    {
        return pimpl->parse(arguments, has_program_name_as_first_arg);
    }
    catch (...)
    {
        return false;
    }
}

bool args::parse(const std::string& arguments, const bool has_program_name_as_first_arg) const noexcept
{
    if (arguments.empty())
    {
        return false;
    }

    // Split string into tokens
    std::vector<std::string> tokens;
    std::istringstream iss(arguments);
    std::string token;

    while (iss >> std::quoted(token))
    {
        tokens.push_back(token);
    }

    return parse(tokens, has_program_name_as_first_arg);
}

bool args::parse(int argc, char** argv, const bool has_program_name_as_first_arg) const noexcept
{
    if (argc <= 0 || !argv)
    {
        return false;
    }

    std::vector<std::string> args_vec;
    args_vec.reserve(argc);

    for (int i = 0; i < argc; ++i)
    {
        if (argv[i])
        {
            args_vec.emplace_back(argv[i]);
        }
    }

    return parse(args_vec, has_program_name_as_first_arg);
}

void args::clear() const noexcept
{
    if (pimpl)
    {
        pimpl->clear();
    }
}

std::optional<std::string> args::get(const std::string& key) const noexcept
{
    if (!pimpl || pimpl->arguments.empty())
    {
        return std::nullopt;
    }

    // Search in first map (command-line args or first JSON object)
    const auto& map = pimpl->arguments.front();
    if (const auto it = map.find(key); it != map.cend())
    {
        return it->second;
    }

    return std::nullopt;
}

std::optional<std::unordered_map<std::string, std::string>> args::get() const noexcept
{
    if (!pimpl || pimpl->arguments.empty())
    {
        return std::nullopt;
    }

    return pimpl->arguments.front();
}

std::optional<std::vector<std::unordered_map<std::string, std::string>>> args::get_array() const noexcept
{
    if (!pimpl || pimpl->arguments.empty())
    {
        return std::nullopt;
    }

    return pimpl->arguments;
}
