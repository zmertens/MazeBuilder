#include <MazeBuilder/parsing_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/configurator.h>
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
#include <variant>

#include <fmt/format.h>

using namespace mazes;

namespace
{
    struct token_cursor
    {
        std::span<const std::string> tokens;
        std::size_t pos{0};

        [[nodiscard]] bool done() const noexcept { return pos >= tokens.size(); }
        [[nodiscard]] std::string_view peek() const noexcept { return tokens[pos]; }
        std::string_view take() noexcept { return tokens[pos++]; }
    };

    bool looks_like_option(std::string_view token) noexcept
    {
        return token.starts_with('-');
    }

    /// Contract: a sub-state claims a token via matches(), then parse() consumes
    /// the key (and its value) from the cursor and writes into out.
    class parse_sub_state
    {
    public:
        virtual ~parse_sub_state() = default;
        [[nodiscard]] virtual bool matches(std::string_view token) const noexcept = 0;
        [[nodiscard]] virtual bool parse(token_cursor &in, arg_map &out) const = 0;
    };

    // Program name (or an ignored leading positional when none is declared).
    class parse_app_state final : public parse_sub_state
    {
    public:
        explicit parse_app_state(bool has_program_name) noexcept : has_program_name{has_program_name} {}

        bool matches(std::string_view token) const noexcept override
        {
            return has_program_name || !looks_like_option(token);
        }

        bool parse(token_cursor &in, arg_map &out) const override
        {
            const auto token = in.take();
            if (has_program_name)
            {
                out[args::APP_KEY] = token;
            }
            return true;
        }

    private:
        bool has_program_name;
    };

    // Handles "-x value", "-xvalue", "--long value" and "--long=value".
    class keyed_sub_state : public parse_sub_state
    {
    public:
        keyed_sub_state(std::string_view word, std::string_view flag, std::string_view option) noexcept
            : word_{word}, flag_{flag}, option_{option}
        {
        }

        [[nodiscard]] bool matches(std::string_view token) const noexcept override
        {
            return token == flag_ || token == option_ || (token.size() > flag_.size() && token.starts_with(flag_)) ||
                   has_inline_value(token);
        }

        [[nodiscard]] bool parse(token_cursor &in, arg_map &out) const override
        {
            const auto token = in.take();
            std::string_view value;
            if (token == flag_ || token == option_)
            {
                if (in.done() || looks_like_option(in.peek()))
                {
                    return false;
                }
                value = in.take();
            }
            else
            {
                value = inline_value(token);
                // "--opt=" followed by the value as its own token
                if (value.empty() && has_inline_value(token) && !in.done() && !looks_like_option(in.peek()))
                {
                    value = in.take();
                }
            }

            if (value.empty() || !accepts(value))
            {
                return false;
            }
            store(out, value);
            return true;
        }

        [[nodiscard]] bool owns_key(std::string_view key) const noexcept
        {
            return key == word_ || key == flag_ || key == option_;
        }

        [[nodiscard]] virtual bool accepts(std::string_view) const noexcept { return true; }

        // Later occurrences overwrite earlier ones.
        void store(arg_map &out, std::string_view value) const
        {
            out[std::string{flag_}] = value;
            out[std::string{option_}] = value;
            out[std::string{word_}] = value;
        }

    protected:
        [[nodiscard]] bool has_inline_value(std::string_view token) const noexcept
        {
            return token.size() > option_.size() && token.starts_with(option_) && token[option_.size()] == '=';
        }

        // Value glued to the flag ("-r10") or following '=' ("--rows=10").
        [[nodiscard]] std::string_view inline_value(std::string_view token) const noexcept
        {
            return has_inline_value(token) ? token.substr(option_.size() + 1) : token.substr(flag_.size());
        }

        std::string_view word_;
        std::string_view flag_;
        std::string_view option_;
    };

    class numeric_sub_state : public keyed_sub_state
    {
    public:
        using keyed_sub_state::keyed_sub_state;

        [[nodiscard]] bool accepts(std::string_view value) const noexcept override
        {
            return std::ranges::all_of(value, [](unsigned char c)
                                       { return std::isdigit(c) != 0; });
        }
    };

    class parse_row_state final : public numeric_sub_state
    {
    public:
        parse_row_state() noexcept : numeric_sub_state{args::ROW_WORD_STR, args::ROW_FLAG_STR, args::ROW_OPTION_STR} {}
    };

    class parse_column_state final : public numeric_sub_state
    {
    public:
        parse_column_state() noexcept
            : numeric_sub_state{args::COLUMN_WORD_STR, args::COLUMN_FLAG_STR, args::COLUMN_OPTION_STR}
        {
        }
    };

    class parse_level_state final : public numeric_sub_state
    {
    public:
        parse_level_state() noexcept
            : numeric_sub_state{args::LEVEL_WORD_STR, args::LEVEL_FLAG_STR, args::LEVEL_OPTION_STR}
        {
        }
    };

    class parse_seed_state final : public numeric_sub_state
    {
    public:
        parse_seed_state() noexcept
            : numeric_sub_state{args::SEED_WORD_STR, args::SEED_FLAG_STR, args::SEED_OPTION_STR}
        {
        }
    };

    class parse_algo_state final : public keyed_sub_state
    {
    public:
        parse_algo_state() noexcept
            : keyed_sub_state{args::ALGO_ID_WORD_STR, args::ALGO_ID_FLAG_STR, args::ALGO_ID_OPTION_STR}
        {
        }

        [[nodiscard]] bool accepts(std::string_view value) const noexcept override
        {
            try
            {
                (void)to_algo_from_sv(value);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }
    };

    class parse_output_state final : public keyed_sub_state
    {
    public:
        parse_output_state() noexcept
            : keyed_sub_state{args::OUTPUT_ID_WORD_STR, args::OUTPUT_ID_FLAG_STR, args::OUTPUT_ID_OPTION_STR}
        {
        }

        [[nodiscard]] bool accepts(std::string_view value) const noexcept override
        {
            try
            {
                const auto extension = string_utils::f_ext_str(value);
                (void)to_output_format_from_sv(extension.empty() ? std::string{value} : extension);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }
    };

    // The value is a file path or a backtick-wrapped JSON string; parse_args expands it.
    class parse_json_state final : public keyed_sub_state
    {
    public:
        parse_json_state() noexcept
            : keyed_sub_state{args::JSON_WORD_STR, args::JSON_FLAG_STR, args::JSON_OPTION_STR}
        {
        }
    };

    class parse_mask_state final : public keyed_sub_state
    {
    public:
        parse_mask_state() noexcept
            : keyed_sub_state{args::MASK_WORD_STR, args::MASK_FLAG_STR, args::MASK_OPTION_STR}
        {
        }
    };

    // Bare flag means "true"; otherwise requires a slice such as [0:10].
    class parse_distances_state final : public keyed_sub_state
    {
    public:
        parse_distances_state() noexcept
            : keyed_sub_state{args::DISTANCES_WORD_STR, args::DISTANCES_FLAG_STR, args::DISTANCES_OPTION_STR}
        {
        }

        [[nodiscard]] bool parse(token_cursor &in, arg_map &out) const override
        {
            const auto token = in.take();
            std::string_view slice;
            if (token == flag_ || token == option_)
            {
                if (in.done() || looks_like_option(in.peek()))
                {
                    store(out, args::TRUE_VALUE);
                    return true;
                }
                slice = in.take();
            }
            else
            {
                slice = inline_value(token);
            }
            return store_slice(out, slice);
        }

    private:
        bool store_slice(arg_map &out, std::string_view slice) const
        {
            static const std::regex slice_pattern(R"(\[(\d*):(-?\d*)\])");

            std::match_results<std::string_view::const_iterator> m;
            if (!std::regex_match(slice.cbegin(), slice.cend(), m, slice_pattern))
            {
                async_logger::instance()->log(fmt::format("Invalid slice notation: {}", slice));
                return false;
            }

            int start = configurator::DEFAULT_DISTANCES_START;
            int end = configurator::DEFAULT_DISTANCES_END;
            if (m[1].length() > 0)
            {
                std::from_chars(&*m[1].first, &*m[1].first + m[1].length(), start);
            }
            if (m[2].length() > 0)
            {
                std::from_chars(&*m[2].first, &*m[2].first + m[2].length(), end);
            }

            store(out, fmt::format("[{}:{}]", start, end));
            out[args::DISTANCES_START_VAL_STR] = std::to_string(start);
            out[args::DISTANCES_END_VAL_STR] = std::to_string(end);
            return true;
        }
    };

    // Flags that never take a value: -h/--help, -v/--version.
    class parse_standalone_state final : public keyed_sub_state
    {
    public:
        using keyed_sub_state::keyed_sub_state;

        [[nodiscard]] bool matches(std::string_view token) const noexcept override
        {
            return token == flag_ || token == option_;
        }

        [[nodiscard]] bool parse(token_cursor &in, arg_map &out) const override
        {
            in.take();
            store(out, args::TRUE_VALUE);
            return true;
        }
    };

    using sub_state_list = std::vector<std::unique_ptr<keyed_sub_state>>;

    sub_state_list make_sub_states()
    {
        sub_state_list states;
        states.push_back(std::make_unique<parse_row_state>());
        states.push_back(std::make_unique<parse_column_state>());
        states.push_back(std::make_unique<parse_level_state>());
        states.push_back(std::make_unique<parse_seed_state>());
        states.push_back(std::make_unique<parse_algo_state>());
        states.push_back(std::make_unique<parse_output_state>());
        states.push_back(std::make_unique<parse_json_state>());
        states.push_back(std::make_unique<parse_mask_state>());
        states.push_back(std::make_unique<parse_distances_state>());
        states.push_back(std::make_unique<parse_standalone_state>("help", "-h", "--help"));
        states.push_back(std::make_unique<parse_standalone_state>("version", "-v", "--version"));
        return states;
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
            cwd.parent_path().parent_path() / "tests",
            raw_path.has_parent_path() ? raw_path.parent_path() : cwd};

        for (const auto &root : search_roots)
        {
            if (const auto candidate_path = root / raw_path; std::filesystem::exists(candidate_path))
            {
                return candidate_path;
            }
        }

        if (raw_path.has_parent_path())
        {
            return raw_path;
        }

        for (const auto &root : search_roots)
        {
            if (const auto candidate_path = root / raw_path.filename(); std::filesystem::exists(candidate_path))
            {
                return candidate_path;
            }
        }

        return raw_path;
    }

    std::vector<arg_map> load_json_objects(const std::string &json_input)
    {
        const std::string trimmed{string_utils::strip_whitespace(json_input)};
        const std::string text{string_utils::strip_backticks(trimmed)};
        const auto path = resolve_json_file_path(text);
        const bool is_file = !text.empty() && std::filesystem::exists(path);

        json_helper jh{};
        arg_map single;
        std::vector<arg_map> array;
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

    // Each JSON object becomes its own map, validated by the same sub-states as the command line.
    bool expand_json(const sub_state_list &states, const std::string &json_input, std::vector<arg_map> &maps)
    {
        for (const auto &object : load_json_objects(json_input))
        {
            if (object.empty())
            {
                continue;
            }

            arg_map m;
            for (const auto &[key, value] : object)
            {
                const auto owner = std::ranges::find_if(states, [&key](const auto &s)
                                                        { return s->owns_key(key); });
                if (owner == states.cend())
                {
                    m[key] = value;
                }
                else if ((*owner)->accepts(value))
                {
                    (*owner)->store(m, value);
                }
                else
                {
                    return false;
                }
            }

            m[args::JSON_FLAG_STR] = json_input;
            m[args::JSON_OPTION_STR] = json_input;
            m[args::JSON_WORD_STR] = json_input;
            maps.push_back(std::move(m));
        }
        return !maps.empty();
    }
} // namespace

parse_result mazes::parse_args(std::span<const std::string> tokens, bool has_program_name) noexcept
{
    parse_result result;
    try
    {
        token_cursor cursor{tokens};
        arg_map flat;
        const auto states = make_sub_states();

        if (const parse_app_state app{has_program_name}; !cursor.done() && app.matches(cursor.peek()))
        {
            (void)app.parse(cursor, flat);
        }

        while (!cursor.done())
        {
            const std::string token{cursor.peek()};
            const auto state_it = std::ranges::find_if(states, [&token](const auto &s)
                                                       { return s->matches(token); });
            if (state_it == states.cend())
            {
                result.error = fmt::format("Unrecognized argument '{}'", token);
                return result;
            }
            if (!(*state_it)->parse(cursor, flat))
            {
                result.error = fmt::format("Invalid or missing value for '{}'", token);
                return result;
            }
        }

        if (const auto json_it = flat.find(args::JSON_WORD_STR); json_it != flat.cend())
        {
            if (!expand_json(states, json_it->second, result.maps))
            {
                result.maps.clear();
                result.error = "Invalid JSON input";
                return result;
            }
        }
        else
        {
            result.maps.push_back(std::move(flat));
        }

        result.ok = !result.maps.front().empty();
        if (!result.ok)
        {
            result.maps.clear();
            result.error = "No arguments found";
        }
    }
    catch (const std::exception &e)
    {
        result.maps.clear();
        result.error = e.what();
    }
    return result;
}

parsing_state::parsing_state(const runtime_app::context &ctx, runtime_stack *rs)
    : state(ctx, rs), args_mapper{ctx.get_args_manager()}, processed_text_mapper{ctx.get_text_manager()}
{
    state_utils::validate_mappers(args_mapper, processed_text_mapper);
}

std::optional<args> parsing_state::convert(const std::string_view arguments) const noexcept
{
    if (arguments.empty())
    {
        return std::nullopt;
    }

    std::optional<args> result{std::in_place};
    if (!result->parse(std::string{arguments}))
    {
        result.reset();
    }
    return result;
}

void parsing_state::draw() const noexcept
{
    // No visual output for now
}

bool parsing_state::update(randomizer &rng, [[maybe_unused]] double delta_time) noexcept
{
    if (!args_mapper || !processed_text_mapper)
    {
        request_stack_pop();
        return false;
    }

    args *parsed_args = nullptr;
    try
    {
        parsed_args = &args_mapper->get(args_identifier::PARSED);
    }
    catch (...)
    {
        request_stack_pop();
        return false;
    }

    if (parsed_args->count() == 0)
    {
        processed_text *unknown_text = nullptr;
        try
        {
            unknown_text = &processed_text_mapper->get(processed_text_identifier::UNKNOWN);
        }
        catch (...)
        {
            request_stack_pop();
            return false;
        }

        const auto unknown_processed = unknown_text->get();
        const auto *raw_input = std::get_if<std::string>(&unknown_processed);
        if (!raw_input || raw_input->empty())
        {
            request_stack_pop();
            return false;
        }

        auto parsed_input = convert(*raw_input);

        // Consumed; clear so a subsequent apply() call can supply fresh input.
        unknown_text->set_processed(std::monostate{});

        if (!parsed_input.has_value())
        {
            request_stack_pop();
            return false;
        }

        try
        {
            args_mapper->get(args_identifier::RAW) = *parsed_input;
            *parsed_args = std::move(*parsed_input);
        }
        catch (...)
        {
            request_stack_pop();
            return false;
        }
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
