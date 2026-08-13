#include <MazeBuilder/args.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/string_utils.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <ranges>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace mazes;

// token and tiny_parser Implementation
namespace
{
    static const auto SLICE_REGEX = std::regex(R"(\[\s*-?\d*\s*:\s*-?\d*\s*\])");

    // Word-form keys recognized by this parser.
    bool is_known_word(std::string_view word_key) noexcept
    {
        static const std::unordered_set<std::string_view> known = {
            args::ROW_WORD_STR, args::COLUMN_WORD_STR, args::LEVEL_WORD_STR, args::SEED_WORD_STR,
            args::ALGO_ID_WORD_STR, args::OUTPUT_ID_WORD_STR, args::JSON_WORD_STR,
            args::DISTANCES_WORD_STR, args::MASK_WORD_STR};
        return known.contains(word_key);
    }

    // Fields that must hold digits only.
    bool is_numeric_word(std::string_view word_key) noexcept
    {
        return word_key == args::ROW_WORD_STR || word_key == args::COLUMN_WORD_STR ||
               word_key == args::LEVEL_WORD_STR || word_key == args::SEED_WORD_STR;
    }

    bool is_valid_numeric_value(std::string_view value) noexcept
    {
        return !value.empty() && std::ranges::all_of(value, [](unsigned char c)
                                                       { return std::isdigit(c) != 0; });
    }

    // Flags that may stand alone (no following value) even though unrecognized.
    bool is_standalone_flag(std::string_view word_key) noexcept
    {
        return word_key == args::DISTANCES_WORD_STR || word_key == "-h" || word_key == "--help" ||
               word_key == "-v" || word_key == "--version";
    }

    enum class TokenType
    {
        PROGRAM,
        // -r, -c, etc.
        SHORT_FLAG,
        // --rows, --columns, etc.
        LONG_OPTION,
        VALUE,
        // [0:10]
        SLICE_NOTATION,
        END_OF_INPUT
    };

    struct token
    {
        TokenType type;
        std::string text;
    };

    // tiny_tokenizer: converts input string vector into token stream
    class tiny_tokenizer
    {
    public:
        explicit tiny_tokenizer(const std::vector<std::string> &args, const bool skip_first = false)
            : current_args(args), token_idx(skip_first ? 1 : 0), has_program_name(skip_first)
        {
        }

        token next()
        {
            if (token_idx >= current_args.size())
            {
                return token{.type = TokenType::END_OF_INPUT, .text = ""};
            }

            const auto &arg = current_args.at(token_idx++);

            // Check for slice notation: must be well-formed [..:..]
            // Properly formatted slices have '[' at start, ']' at end, and exactly one ':' in between
            // Minimum valid slice is [:]  (size 3)
            if (std::regex_match(arg, SLICE_REGEX))
            {
                return token{.type = TokenType::SLICE_NOTATION, .text = arg};
            }

            // Check for long option
            if (!arg.empty() && arg.starts_with("--"))
            {
                return token{.type = TokenType::LONG_OPTION, .text = arg};
            }
            else if (arg.starts_with("-") && arg.size() >= 2)
            {
                if (std::isalpha(static_cast<std::uint16_t>(arg.at(1))))
                {
                    return token{.type = TokenType::SHORT_FLAG, .text = arg};
                }
            }
            else if (token_idx == 1 && has_program_name)
            {
                // First argument is program name (if has_program_name is true)
                return token{.type = TokenType::PROGRAM, .text = arg};
            }

            return token{.type = TokenType::VALUE, .text = arg};
        }

        [[nodiscard]] bool has_more() const { return token_idx < current_args.size(); }

        void unread() noexcept
        {
            if (token_idx > 0)
            {
                --token_idx;
            }
        }

    private:
        const std::vector<std::string> &current_args;
        size_t token_idx;
        bool has_program_name;
    };

    // tiny_parser: processes tokens and builds argument map
    class tiny_parser
    {
    public:
        using ArgMap = std::unordered_map<std::string, std::string>;
        using token = ::token;

        tiny_parser() = default;

        bool parse(const std::vector<std::string> &args, bool has_program_name)
        {
            tiny_tokenizer tokenizer{args, has_program_name};
            parsed_results.clear();
            word_map.clear();

            try
            {
                // Handle program name if present
                if (has_program_name && !args.empty())
                {
                    store_value(args::APP_KEY, args.at(0));
                }

                bool first_token = true;

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
                        first_token = false;
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
                    else if (token.type == TokenType::VALUE)
                    {
                        // A bare/malformed value is only tolerated as an implicit leading
                        // program name when the caller did not already declare one.
                        if (!(first_token && !has_program_name))
                        {
                            return false;
                        }
                    }

                    first_token = false;
                }

                // Post-processing: if we parsed JSON, delegate to JSON handler
                bool json_ok = true;
                if (auto it = word_map.find(args::JSON_WORD_STR); it != word_map.cend())
                {
                    json_ok = process_json(it->second);
                }

                // Preserve whatever was parsed so far (even on JSON failure) for inspection,
                // unless process_json already replaced parsed_results (JSON array case).
                if (parsed_results.empty())
                {
                    parsed_results.push_back(std::move(word_map));
                }

                return json_ok && !parsed_results.empty() && !parsed_results.front().empty();
            }
            catch (const std::exception &e)
            {
                global_async_logger().log("Parse error: {}\n", e.what());
                return false;
            }
        }

        [[nodiscard]] const std::vector<ArgMap> &results() const { return parsed_results; }

    private:
        ArgMap word_map;
        std::vector<ArgMap> parsed_results;

        // Store value with automatic aliasing (flag/option/word forms)
        void store_value(std::string_view key, std::string_view value)
        {
            // Map of word-form keys to their flag and option aliases
            static const std::unordered_map<std::string_view, std::tuple<
                                                                  std::string_view, std::string_view, std::string_view>>
                aliases = {
                    {args::ROW_WORD_STR, {args::ROW_FLAG_STR, args::ROW_OPTION_STR, args::ROW_WORD_STR}},
                    {args::COLUMN_WORD_STR, {args::COLUMN_FLAG_STR, args::COLUMN_OPTION_STR, args::COLUMN_WORD_STR}},
                    {args::LEVEL_WORD_STR, {args::LEVEL_FLAG_STR, args::LEVEL_OPTION_STR, args::LEVEL_WORD_STR}},
                    {args::SEED_WORD_STR, {args::SEED_FLAG_STR, args::SEED_OPTION_STR, args::SEED_WORD_STR}},
                    {args::ALGO_ID_WORD_STR, {args::ALGO_ID_FLAG_STR, args::ALGO_ID_OPTION_STR, args::ALGO_ID_WORD_STR}},
                    {args::OUTPUT_ID_WORD_STR,
                     {args::OUTPUT_ID_FLAG_STR, args::OUTPUT_ID_OPTION_STR, args::OUTPUT_ID_WORD_STR}},
                    {args::JSON_WORD_STR, {args::JSON_FLAG_STR, args::JSON_OPTION_STR, args::JSON_WORD_STR}},
                    {args::DISTANCES_WORD_STR,
                     {args::DISTANCES_FLAG_STR, args::DISTANCES_OPTION_STR, args::DISTANCES_WORD_STR}},
                    {args::MASK_WORD_STR, {args::MASK_FLAG_STR, args::MASK_OPTION_STR, args::MASK_WORD_STR}}};

            std::string_view word_key = key;

            // Find the alias tuple for this key
            auto it = aliases.find(word_key);
            if (it != aliases.cend())
            {
                const auto &[flag, option, word] = it->second;
                if (!flag.empty())
                {
                    word_map[std::string(flag)] = value;
                }
                if (!option.empty())
                {
                    word_map[std::string(option)] = value;
                }
                word_map[std::string(word)] = value;
            }
            else
            {
                // No alias, just store as-is
                word_map[std::string(key)] = value;
            }
        }

        // Normalize flag/option to word-form key
        static std::string_view normalize_key(std::string_view key)
        {
            static const std::unordered_map<std::string_view, std::string_view> normalization = {
                {args::ROW_FLAG_STR, args::ROW_WORD_STR}, {args::ROW_OPTION_STR, args::ROW_WORD_STR}, {args::COLUMN_FLAG_STR, args::COLUMN_WORD_STR}, {args::COLUMN_OPTION_STR, args::COLUMN_WORD_STR}, {args::LEVEL_FLAG_STR, args::LEVEL_WORD_STR}, {args::LEVEL_OPTION_STR, args::LEVEL_WORD_STR}, {args::SEED_FLAG_STR, args::SEED_WORD_STR}, {args::SEED_OPTION_STR, args::SEED_WORD_STR}, {args::ALGO_ID_FLAG_STR, args::ALGO_ID_WORD_STR}, {args::ALGO_ID_OPTION_STR, args::ALGO_ID_WORD_STR}, {args::OUTPUT_ID_FLAG_STR, args::OUTPUT_ID_WORD_STR}, {args::OUTPUT_ID_OPTION_STR, args::OUTPUT_ID_WORD_STR}, {args::JSON_FLAG_STR, args::JSON_WORD_STR}, {args::JSON_OPTION_STR, args::JSON_WORD_STR}, {args::DISTANCES_FLAG_STR, args::DISTANCES_WORD_STR}, {args::DISTANCES_OPTION_STR, args::DISTANCES_WORD_STR}, {args::MASK_FLAG_STR, args::MASK_WORD_STR}, {args::MASK_OPTION_STR, args::MASK_WORD_STR}};

            if (auto it = normalization.find(key); it != normalization.cend())
            {
                return it->second;
            }
            return key;
        }

        bool parse_short_flag(const token &token, tiny_tokenizer &tokenizer)
        {
            std::string flag = token.text;

            if (flag.size() > 2)
            {
                std::string_view key(flag.data(), 2);
                std::string_view value(flag.data() + 2, flag.size() - 2);

                auto word_key = normalize_key(key);

                // Special case: -d[...] distances with slice
                if (word_key == args::DISTANCES_WORD_STR && value.starts_with('['))
                {
                    return parse_distances_value(word_key, std::string(value));
                }

                // Only algo, numeric, and json (file path) flags support a value glued directly onto the flag letter.
                if (word_key == args::ALGO_ID_WORD_STR)
                {
                    try
                    {
                        (void)to_algo_from_sv(value);
                    }
                    catch (...)
                    {
                        return false;
                    }
                }
                else if (word_key != args::JSON_WORD_STR && !is_numeric_word(word_key))
                {
                    return false;
                }

                store_value(word_key, value);
                return true;
            }

            // Boolean flags (no value expected)
            auto word_key = normalize_key(flag);

            if (!is_known_word(word_key) && !is_standalone_flag(word_key))
            {
                return false;
            }

            // Flag with value: check for next token
            if (!tokenizer.has_more())
            {
                // Distances/help/version flags can be standalone
                if (is_standalone_flag(word_key))
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

            if (is_standalone_flag(word_key) &&
                (value_token.type == TokenType::SHORT_FLAG || value_token.type == TokenType::LONG_OPTION))
            {
                tokenizer.unread();
                store_value(word_key, args::TRUE_VALUE);
                return true;
            }

            // Numeric fields must contain digits only when passed as a separate token.
            if (is_numeric_word(word_key) && value_token.type == TokenType::VALUE &&
                !is_valid_numeric_value(value_token.text))
            {
                return false;
            }

            // Regular value
            store_value(word_key, value_token.text);
            return true;
        }

        // Parse long option: --help, --rows=10, --rows 10
        bool parse_long_option(const token &token, tiny_tokenizer &tokenizer)
        {
            std::string option = token.text;

            // Check for embedded value (--rows=10)
            if (const auto eq_pos = option.find('='); eq_pos != std::string::npos)
            {
                const std::string_view key(option.data(), eq_pos);
                const std::string_view value(option.data() + eq_pos + 1, option.size() - eq_pos - 1);

                const auto word_key = normalize_key(key);

                if (!is_known_word(word_key))
                {
                    return false;
                }

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

                // Numeric fields must contain digits only.
                if (is_numeric_word(word_key) && !is_valid_numeric_value(value))
                {
                    return false;
                }

                store_value(word_key, value);
                return true;
            }

            // Boolean options (no value expected)
            const auto word_key = normalize_key(option);

            if (!is_known_word(word_key) && !is_standalone_flag(word_key))
            {
                return false;
            }

            // Option with value: check for next token
            if (!tokenizer.has_more())
            {
                // Distances/help/version can be standalone
                if (is_standalone_flag(word_key))
                {
                    store_value(word_key, args::TRUE_VALUE);
                    return true;
                }
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

            if (is_standalone_flag(word_key) && (type == TokenType::SHORT_FLAG || type == TokenType::LONG_OPTION))
            {
                tokenizer.unread();
                store_value(word_key, args::TRUE_VALUE);
                return true;
            }

            // Numeric fields must contain digits only when passed as a separate token.
            if (is_numeric_word(word_key) && type == TokenType::VALUE && !is_valid_numeric_value(text))
            {
                return false;
            }

            // Regular value
            store_value(word_key, text);
            return true;
        }

        // Parse distances value with slice notation [start:end]
        bool parse_distances_value(const std::string_view key, const std::string &value)
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

                word_map[args::DISTANCES_START_VAL_STR] = std::to_string(start);
                word_map[args::DISTANCES_END_VAL_STR] = std::to_string(end);

                return true;
            }

#ifdef MAZE_DEBUG
            global_async_logger().log("Invalid slice notation: {}", value);
#endif
            return false;
        }

        // Process JSON input (file or string)
        bool process_json(const std::string &json_input)
        {
            try
            {
                // Strip whitespace and check format
                auto trimmed = std::string{string_utils::strip_whitespace(json_input)};

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
                    for (const auto &[k, v] : parsed_json)
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
                    throw std::runtime_error("File not found: " + json_input);
                }

                auto resolved_str = resolved_path.string();

                // Try loading as array first
                std::vector<std::unordered_map<std::string, std::string>> parsed_array;
                if (jh.load_array(resolved_str, parsed_array))
                {
                    parsed_results.clear();

                    for (const auto &json_obj : parsed_array)
                    {
                        ArgMap map;
                        for (const auto &[k, v] : json_obj)
                        {
                            auto word_key = normalize_key(k);

                            // Use aliasing for known keys
                            static const std::unordered_map<
                                std::string_view, std::tuple<std::string_view, std::string_view, std::string_view>>
                                aliases = {
                                    {args::ROW_WORD_STR,
                                     {args::ROW_FLAG_STR, args::ROW_OPTION_STR, args::ROW_WORD_STR}},
                                    {args::COLUMN_WORD_STR,
                                     {args::COLUMN_FLAG_STR, args::COLUMN_OPTION_STR, args::COLUMN_WORD_STR}},
                                    {args::LEVEL_WORD_STR,
                                     {args::LEVEL_FLAG_STR, args::LEVEL_OPTION_STR, args::LEVEL_WORD_STR}},
                                    {args::SEED_WORD_STR,
                                     {args::SEED_FLAG_STR, args::SEED_OPTION_STR, args::SEED_WORD_STR}},
                                    {args::ALGO_ID_WORD_STR,
                                     {args::ALGO_ID_FLAG_STR, args::ALGO_ID_OPTION_STR, args::ALGO_ID_WORD_STR}},
                                    {args::OUTPUT_ID_WORD_STR,
                                     {args::OUTPUT_ID_FLAG_STR, args::OUTPUT_ID_OPTION_STR, args::OUTPUT_ID_WORD_STR}},
                                    {args::DISTANCES_WORD_STR,
                                     {args::DISTANCES_FLAG_STR, args::DISTANCES_OPTION_STR, args::DISTANCES_WORD_STR}}};

                            if (auto it = aliases.find(word_key); it != aliases.cend())
                            {
                                const auto &[flag, option, word] = it->second;
                                if (!flag.empty())
                                {
                                    map[std::string{flag}] = v;
                                }
                                if (!option.empty())
                                {
                                    map[std::string{option}] = v;
                                }
                                map[std::string{word}] = v;
                            }
                            else
                            {
                                map[std::string{k}] = v;
                            }
                        }

                        // Add JSON keys
                        map[args::JSON_FLAG_STR] = json_input;
                        map[args::JSON_OPTION_STR] = json_input;
                        map[args::JSON_WORD_STR] = json_input;

                        parsed_results.push_back(std::move(map));
                    }

                    return true;
                }

                // Try loading as single object
                if (std::unordered_map<std::string, std::string> parsed_json; jh.load(std::cref(resolved_str), std::ref(parsed_json)))
                {
                    for (const auto &[k, v] : parsed_json)
                    {
                        auto word_key = normalize_key(k);
                        store_value(word_key, v);
                    }
                    return true;
                }

                return false;
            }
            catch (const std::exception &e)
            {
                global_async_logger().log("JSON processing error: {}", e.what());
                return false;
            }
        }
    };
} // anonymous namespace

class args::impl
{
public:
    impl() = default;

    std::vector<std::unordered_map<std::string, std::string>> arguments;

    bool parse(const std::vector<std::string> &args_vec, const bool has_program_name)
    {
        static tiny_parser parser;
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

args::args() noexcept : pimpl{std::make_unique<impl>()}
{
}

args::~args() = default;

args::args(args &&other) noexcept = default;

args &args::operator=(args &&other) noexcept = default;

args::args(const args &other) : pimpl{std::make_unique<impl>()}
{
    if (other.pimpl)
    {
        pimpl->arguments = other.pimpl->arguments;
    }
}

args &args::operator=(const args &other)
{
    if (this != &other && other.pimpl)
    {
        pimpl = std::make_unique<impl>();
        pimpl->arguments = other.pimpl->arguments;
    }
    return *this;
}

bool args::parse(const std::vector<std::string> &arguments, const bool has_program_name_as_first_arg) const noexcept
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

bool args::parse(const std::string &arguments, const bool has_program_name_as_first_arg) const noexcept
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

bool args::parse(int argc, char **argv, const bool has_program_name_as_first_arg) const noexcept
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

std::optional<std::string> args::get(const std::string &key) const noexcept
{
    if (!pimpl || pimpl->arguments.empty())
    {
        return std::nullopt;
    }

    // Search in first map (command-line args or first JSON object)
    const auto &map = pimpl->arguments.front();
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
