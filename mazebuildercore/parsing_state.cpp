#include <MazeBuilder/parsing_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/state_utils.h>

#include <optional>
#include <variant>

#include <fmt/format.h>

using namespace mazes;

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

bool parsing_state::update([[maybe_unused]] double delta_time) noexcept
{
    if (!args_mapper || !processed_text_mapper)
    {
        request_stack_pop();
        return false;
    }

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

    auto parsed_args = convert(*raw_input);

    // Consumed; clear so a subsequent apply() call can supply fresh input.
    unknown_text->set_processed(std::monostate{});

    if (!parsed_args.has_value())
    {
        request_stack_pop();
        return false;
    }

    try
    {
        args_mapper->get(args_identifier::RAW) = *parsed_args;
        args_mapper->get(args_identifier::PARSED) = *parsed_args;
    }
    catch (...)
    {
        request_stack_pop();
        return false;
    }

    // Determine which maze algorithm state to run first.
    state::ID next_state = state::ID::BINARY_TREE;

    if (const auto parsed = parsed_args->get(); parsed.has_value())
    {
        if (const auto it = parsed->find(mazes::args::ALGO_ID_WORD_STR); it != parsed->cend())
        {
            try
            {
                switch (to_algo_from_sv(it->second))
                {
                case algo::BINARY_TREE:
                    next_state = state::ID::BINARY_TREE;
                    break;
                case algo::DFS:
                    next_state = state::ID::DFS;
                    break;
                case algo::SIDEWINDER:
                    next_state = state::ID::SIDEWINDER;
                    break;
                default:
                    next_state = state::ID::BINARY_TREE;
                    break;
                }
            }
            catch (...)
            {
                next_state = state::ID::BINARY_TREE;
            }
        }
    }

    request_stack_pop();
    request_stack_push(next_state);
    return false;
}
