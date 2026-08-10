#include <MazeBuilder/parsing_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/state_utils.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/string_utils.h>

#include <any>

#include <fmt/format.h>

using namespace mazes;

parsing_state::parsing_state(const runtime_app::context &ctx, runtime_stack *rs)
    : state(ctx, rs), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}
{
}

std::optional<args> parsing_state::convert(const std::string_view arguments) const noexcept
{
    if (arguments.empty())
    {
        return std::nullopt;
    }

    // parse() fills the args object; we return by move to avoid inline ~impl destruction
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

bool parsing_state::update(const std::optional<args> &args, [[maybe_unused]] double delta_time) noexcept
{
    // runtime_app already parses input before entering the state machine.
    // Reuse the parsed args and only fall back to raw input conversion when needed.
    std::optional<mazes::args> parsed_args = args;

    if (!parsed_args.has_value())
    {
        if (const auto *raw_input = get_context().get_raw_input(); raw_input && !raw_input->empty())
        {
            parsed_args = convert(*raw_input);
        }
    }

    if (!parsed_args.has_value() || !processed_text_mapper)
    {
        request_stack_pop();
        return true;
    }

    // Determine which output state to run based on the requested output target.
    const state::ID next_state = state_utils::output_state_for(parsed_args);

    request_stack_pop();
    request_stack_push(next_state);
    return false;
}
