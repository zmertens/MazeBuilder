#include <MazeBuilder/parsing_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/processed_text.h>
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

    // Determine which algo to run and push the matching create state on top of the stack
    state::ID next_state = state::ID::BINARY_TREE;

    if (auto parsed = parsed_args->get(); parsed.has_value())
    {
        if (auto it = parsed->find(mazes::args::OUTPUT_FILENAME_WORD_STR); it != parsed->cend())
        {
            try
            {
                const algo a = to_algo_from_sv(it->second);
                switch (a)
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
                    throw std::logic_error({});
                }
            }
            catch (...)
            {
                next_state = state::ID::BINARY_TREE;
            }

            if (auto found = mazes::string_utils::find(it->second, '.'); found != std::string_view::npos)
            {
                const auto extension = mazes::string_utils::get_file_extension(it->second);
                if (extension == "png" || extension == "bmp" || extension == "jpg" || extension == "jpeg")
                {
                    next_state = state::ID::PIXELIZING;
                }
                else if (extension == "obj")
                {
                    next_state = state::ID::WAVEFRONT_OBJECTIFY;
                }
                else if (extension == "json" || extension == "txt")
                {
                    next_state = state::ID::STRINGIFYING;
                }
            }
            else if (it->second == "stdout")
            {
                next_state = state::ID::STRINGIFYING;
            }
        }
    }

    request_stack_pop();
    request_stack_push(next_state);
    return false;
}
