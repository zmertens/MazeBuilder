#include <MazeBuilder/parsing_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/state_utils.h>

#include <array>
#include <optional>
#include <variant>

#include <fmt/format.h>

using namespace mazes;

parsing_state::parsing_state(const runtime_app::context& ctx, runtime_stack* rs)
    : state(ctx, rs), args_mapper{ ctx.get_args_manager() }, processed_text_mapper{ ctx.get_text_manager() }
{
    state_utils::validate_mappers(args_mapper, processed_text_mapper);
}

std::optional<args> parsing_state::convert(const std::string_view arguments) const noexcept
{
    if (arguments.empty())
    {
        return std::nullopt;
    }

    std::optional<args> result{ std::in_place };
    if (!result->parse(std::string{ arguments }))
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

    args* parsed_args = nullptr;
    try
    {
        parsed_args = &args_mapper->get(args_identifier::PARSED);
    } catch (...)
    {
        request_stack_pop();
        return false;
    }

    if (parsed_args->count() == 0)
    {
        processed_text* unknown_text = nullptr;
        try
        {
            unknown_text = &processed_text_mapper->get(processed_text_identifier::UNKNOWN);
        } catch (...)
        {
            request_stack_pop();
            return false;
        }

        const auto unknown_processed = unknown_text->get();
        const auto* raw_input = std::get_if<std::string>(&unknown_processed);
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
        } catch (...)
        {
            request_stack_pop();
            return false;
        }
    }

    const auto current_args = parsed_args->front();
    request_stack_pop();

    const auto mask_it = current_args.find(mazes::args::MASK_WORD_STR);
    const bool has_mask = mask_it != current_args.cend() && !mask_it->second.empty();

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
            default:
                global_async_logger().log(fmt::format("Parsing update - Unrecognized algo '{}'", it->second));
                break;
            }
        } catch (...)
        {
            global_async_logger().log(fmt::format("Parsing update - Exception: Invalid algo '{}'", it->second));
        }
    }

    return true;
}
