#include <MazeBuilder/parsing_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/output_formats.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/state_utils.h>
#include <MazeBuilder/string_utils.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <filesystem>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <variant>

#include <fmt/format.h>

using namespace mazes;

namespace
{
    struct key_pattern
    {
        std::regex pattern;
        std::string_view word;
    };

    // One short regex per argument: "-a", "--algo" or the bare word
    key_pattern make_pattern(std::string_view flag, std::string_view option, std::string_view word)
    {
        return {std::regex{fmt::format("^({}|{}|{})$", flag, option, word)}, word};
    }

    const std::vector<key_pattern> &key_patterns()
    {
        static const std::vector<key_pattern> patterns{
            make_pattern(args::ALGO_ID_FLAG_STR, args::ALGO_ID_OPTION_STR, args::ALGO_ID_WORD_STR),
            make_pattern(args::ROW_FLAG_STR, args::ROW_OPTION_STR, args::ROW_WORD_STR),
            make_pattern(args::COLUMN_FLAG_STR, args::COLUMN_OPTION_STR, args::COLUMN_WORD_STR),
            make_pattern(args::LEVEL_FLAG_STR, args::LEVEL_OPTION_STR, args::LEVEL_WORD_STR),
            make_pattern(args::OUTPUT_ID_FLAG_STR, args::OUTPUT_ID_OPTION_STR, args::OUTPUT_ID_WORD_STR),
            make_pattern(args::SEED_FLAG_STR, args::SEED_OPTION_STR, args::SEED_WORD_STR),
            make_pattern(args::DISTANCES_FLAG_STR, args::DISTANCES_OPTION_STR, args::DISTANCES_WORD_STR),
            make_pattern(args::MASK_FLAG_STR, args::MASK_OPTION_STR, args::MASK_WORD_STR),
            make_pattern(args::JSON_FLAG_STR, args::JSON_OPTION_STR, args::JSON_WORD_STR),
        };
        return patterns;
    }

    bool is_option(std::string_view token) noexcept
    {
        return token.starts_with('-');
    }

    bool is_numeric_key(std::string_view key) noexcept
    {
        return key == args::ROW_WORD_STR || key == args::COLUMN_WORD_STR || key == args::LEVEL_WORD_STR ||
               key == args::SEED_WORD_STR;
    }

    bool is_valid(std::string_view key, std::string_view value) noexcept
    {
        if (value.empty())
        {
            return false;
        }
        return !is_numeric_key(key) || std::ranges::all_of(value, [](unsigned char c)
                                                           { return std::isdigit(c) != 0; });
    }

    // The quote character that opens a value: at the token start or right after '=' ("--json=`{...}`")
    char opening_quote(std::string_view token) noexcept
    {
        std::size_t pos = 0;
        if (token.starts_with('-'))
        {
            const auto eq = token.find('=');
            if (eq == std::string_view::npos)
            {
                return '\0';
            }
            pos = eq + 1;
        }
        return pos < token.size() && (token[pos] == '"' || token[pos] == '`') ? token[pos] : '\0';
    }

    // Splits on spaces, then re-joins tokens that fall inside an unbalanced "..." or `...` span.
    std::vector<std::string> split_args(const std::string &text)
    {
        std::string normalized{text};
        std::ranges::replace_if(normalized, [](unsigned char c)
                                { return c == '\t' || c == '\r' || c == '\n'; }, ' ');

        std::vector<std::string> raw_tokens;
        string_utils::strsplit(normalized, raw_tokens, ' ');

        std::vector<std::string> tokens;
        char open{'\0'};
        for (auto &token : raw_tokens)
        {
            if (open != '\0')
            {
                tokens.back() += ' ' + token;
            }
            else if (token.empty())
            {
                continue;
            }
            else
            {
                tokens.push_back(std::move(token));
                open = opening_quote(tokens.back());
            }

            if (open != '\0' && std::ranges::count(tokens.back(), open) % 2 == 0)
            {
                open = '\0';
            }
        }
        return tokens;
    }

    // Test-environment friendly lookup of a JSON file relative to common roots.
    std::filesystem::path resolve_json_file_path(std::string_view candidate)
    {
        const std::filesystem::path raw_path{std::string(candidate)};
        if (raw_path.empty() || std::filesystem::exists(raw_path))
        {
            return raw_path;
        }

        const auto cwd = std::filesystem::current_path();
        const std::vector<std::filesystem::path> search_roots = {
            cwd,
            cwd / "tests",
            cwd.parent_path(),
            cwd.parent_path() / "tests",
            cwd.parent_path().parent_path(),
            cwd.parent_path().parent_path() / "tests"};

        for (const auto &root : search_roots)
        {
            if (const auto candidate_path = root / raw_path; std::filesystem::exists(candidate_path))
            {
                return candidate_path;
            }
        }
        return raw_path;
    }

    // The input is a file path or a backtick-wrapped JSON string, holding one object or an array of them
    std::vector<args::arg_map> load_json_objects(const std::string &json_input)
    {
        const std::string text{string_utils::strip_backticks(std::string{string_utils::strip_whitespace(json_input)})};
        const auto path = resolve_json_file_path(text);
        const bool is_file = !text.empty() && std::filesystem::exists(path);

        json_helper jh{};
        args::arg_map single;
        std::vector<args::arg_map> array;
        if (is_file)
        {
            if (jh.load(path.string(), single))
            {
                return {std::move(single)};
            }
            if (jh.load_array(path.string(), array))
            {
                return array;
            }
        }
        else
        {
            if (jh.from(text, single))
            {
                return {std::move(single)};
            }
            if (jh.from_array(text, array))
            {
                return array;
            }
        }
        return {};
    }

    // Each JSON object becomes its own map; empty on invalid JSON or values
    std::vector<args::arg_map> expand_json(const std::string &json_input)
    {
        std::vector<args::arg_map> maps;
        for (auto &object : load_json_objects(json_input))
        {
            if (object.empty())
            {
                continue;
            }
            for (const auto &[key, value] : object)
            {
                if (!is_valid(key, value))
                {
                    return {};
                }
            }

            object[args::JSON_FLAG_STR] = json_input;
            object[args::JSON_OPTION_STR] = json_input;
            object[args::JSON_WORD_STR] = json_input;
            maps.push_back(std::move(object));
        }
        return maps;
    }

    // Fills one arg_map from the canonical string, or one per JSON object; empty on any invalid token.
    std::vector<args::arg_map> parse_canonical(const std::string &canonical)
    {
        const auto tokens = split_args(canonical);

        args::arg_map map;
        for (std::size_t i = 0; i < tokens.size(); ++i)
        {
            const auto &patterns = key_patterns();
            const auto it = std::ranges::find_if(patterns, [&](const auto &p)
                                                 { return std::regex_match(tokens[i], p.pattern); });
            if (it == patterns.cend())
            {
                return {};
            }

            const std::string key{it->word};
            const bool has_value = i + 1 < tokens.size() && !is_option(tokens[i + 1]);
            if (has_value)
            {
                std::string value{tokens[++i]};
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                {
                    value = value.substr(1, value.size() - 2);
                }
                if (!is_valid(key, value))
                {
                    return {};
                }
                map[key] = std::move(value);
            }
            else if (key == args::DISTANCES_WORD_STR)
            {
                // A bare distances flag means "true"
                map[key] = args::TRUE_VALUE;
            }
            else
            {
                return {};
            }
        }

        if (map.empty())
        {
            return {};
        }
        if (const auto json_it = map.find(args::JSON_WORD_STR); json_it != map.cend())
        {
            return expand_json(json_it->second);
        }
        return {std::move(map)};
    }
} // namespace

parsing_state::parsing_state(const runtime_app::context &ctx, runtime_stack *rs)
    : state(ctx, rs), args_mapper{ctx.get_args_manager()}
{
    state_utils::validate_mappers(args_mapper);
}

std::any parsing_state::create(const args &a, [[maybe_unused]] randomizer &rng) noexcept
{
    try
    {
        const auto raw = a.get(args::RAW_WORD_STR);
        if (!raw)
        {
            return {};
        }

        std::string canonical;
        for (const auto &token : split_args(*raw))
        {
            // "--opt=value" becomes "--opt value"
            if (const auto eq = token.find('='); is_option(token) && eq != std::string::npos)
            {
                canonical += fmt::format("{} {} ", token.substr(0, eq), token.substr(eq + 1));
            }
            else
            {
                canonical += token + ' ';
            }
        }

        if (canonical.empty())
        {
            return {};
        }
        canonical.pop_back();
        return canonical;
    }
    catch (...)
    {
        return {};
    }
}

void parsing_state::draw() const noexcept
{
    // No visual output for now
}

bool parsing_state::update(randomizer &rng, [[maybe_unused]] double delta_time) noexcept
{
    args *parsed_args{nullptr};
    try
    {
        parsed_args = &args_mapper->get(args_identifier::PARSED);

        // Already parsed: skip straight to dispatch
        if (parsed_args->count() == 0)
        {
            auto &raw_args = args_mapper->get(args_identifier::RAW);
            const auto canonical = create_as<std::string>(raw_args, rng);
            auto maps = canonical ? parse_canonical(*canonical) : std::vector<args::arg_map>{};

            // Raw input is consumed whether or not it parsed
            raw_args.clear();

            if (maps.empty())
            {
                async_logger::instance()->log("Parsing update - no valid arguments\n");
                request_stack_pop();
                return false;
            }
            parsed_args->set(std::move(maps));
        }
    }
    catch (...)
    {
        request_stack_pop();
        return false;
    }

    request_stack_pop();

    const auto current_args = parsed_args->front();

    if (const auto it = current_args.find(mazes::args::SEED_WORD_STR); it != current_args.cend() && !it->second.empty())
    {
        rng.seed(it->second);
    }

    const auto mask_it = current_args.find(mazes::args::MASK_WORD_STR);
    const bool has_mask = mask_it != current_args.cend() && !mask_it->second.empty();

    // Stack is LIFO: the output state runs after the algorithm state pushed below
    request_stack_push(state_utils::output_state_for(current_args));

    if (has_mask)
    {
        request_stack_push(state::ID::LINK_WITH_MASKED);
    }
    else if (const auto it = current_args.find(mazes::args::ALGO_ID_WORD_STR); it != current_args.cend())
    {
        try
        {
            switch (to_algo_from_sv(it->second))
            {
            case algo::BINARY_TREE:
                request_stack_push(state::ID::LINK_WITH_BINARY_TREE);
                break;
            case algo::DFS:
                request_stack_push(state::ID::LINK_WITH_DFS);
                break;
            case algo::SIDEWINDER:
                request_stack_push(state::ID::LINK_WITH_SIDEWINDER);
                break;
            case algo::PRIMS:
                request_stack_push(state::ID::LINK_WITH_PRIMS);
                break;
            default:
                async_logger::instance()->log(fmt::format("Parsing update - Unrecognized algo '{}'", it->second));
                break;
            }
        }
        catch (...)
        {
            async_logger::instance()->log(fmt::format("Parsing update - Exception: Invalid algo '{}'", it->second));
        }
    }
    else
    {
        request_stack_push(state::ID::LINK_WITH_BINARY_TREE);
    }

    return true;
}
