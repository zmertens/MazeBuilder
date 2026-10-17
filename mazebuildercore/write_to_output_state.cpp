#include <MazeBuilder/write_to_output_state.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/bytes.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create_contract.h>
#include <MazeBuilder/output_formats.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/state_utils.h>

#include <fmt/format.h>

#include <string>
#include <type_traits>
#include <variant>

using namespace mazes;

write_to_output_state::write_to_output_state(const runtime_app::context &ctx, runtime_stack *stack)
    : state(ctx, stack),
      args_mapper{ctx.get_args_manager()},
      grid_mapper{ctx.get_grid_manager()},
      processed_text_mapper{ctx.get_text_manager()},
      current_grid_id{grid_identifier::BASIC}
{
    state_utils::validate_mappers(args_mapper, grid_mapper, processed_text_mapper);
}

write_to_output_state::~write_to_output_state() = default;

configurator write_to_output_state::make_config(const args &a) noexcept
{
    auto cfg = state_utils::build_configurator(a);
    current_grid_id = cfg.distances() ? grid_identifier::DISTANCE : grid_identifier::BASIC;

    try
    {
        const auto [rows, cols, levels] = grid_mapper->get(current_grid_id).operations().get_dimensions();
        cfg.ensure_rows(rows).ensure_columns(cols).ensure_levels(levels);
    }
    catch (...)
    {
    }

    return cfg;
}

void write_to_output_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool write_to_output_state::update(randomizer &rng, [[maybe_unused]] double delta_time) noexcept
{
    const auto &parsed_args{args_mapper->get(args_identifier::PARSED)};
    const auto parsed_args_front = parsed_args.front();

    // Concrete states mix in create_contract alongside this base
    auto *creator = dynamic_cast<create_contract *>(this);
    if (!creator)
    {
        request_stack_pop();
        return false;
    }

    auto out = creator->create_as<std::string>(parsed_args, rng).value_or(std::string{});

    std::string output_target;
    auto stdout_format = output_format::TOTAL;
    if (const auto it = parsed_args_front.find(mazes::args::OUTPUT_ID_WORD_STR); it != parsed_args_front.cend())
    {
        // Bare keywords (stdout, obj, b64) print to stdout instead of writing a file.
        stdout_format = stdout_format_from_sv(it->second);
        output_target = stdout_format != output_format::TOTAL ? "" : it->second;
    }
    if (stdout_format == output_format::B64)
    {
        out = bytes::encode(out);
    }

    // Image states produce no text content but still write their pixels to the target
    if (!output_target.empty())
    {
        if (write_output(output_target, out))
        {
            async_logger::instance()->log("Successfully wrote maze to " + output_target);
            if (out.empty())
            {
                out = "Successfully wrote maze to " + output_target;
            }
        }
        else
        {
            async_logger::instance()->log("Failed to write maze to " + output_target);
        }
    }

    processed_text_mapper->get(processed_text_identifier::FINISHED).set(out);

    request_stack_pop();
    if (state_utils::advance_args(get_context()))
    {
        request_stack_push(state::ID::PARSE);
    }

#if defined(MAZE_DEBUG)

    const auto processing_output{processed_text::to_string(processed_text_mapper->get(processed_text_identifier::PROCESSING).get())};
    async_logger::instance()->log(fmt::format("Processing output:\n{}\n", processing_output));
#endif

    return true;
}
