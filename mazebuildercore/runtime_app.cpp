#include <MazeBuilder/runtime_app.h>

#include <MazeBuilder/bt_maze_create_state.h>
#include <MazeBuilder/dfs_maze_create_state.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/loading_state.h>
#include <MazeBuilder/parsing_state.h>
#include <MazeBuilder/pixels_create_state.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/progress.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/sw_maze_create_state.h>
#include <MazeBuilder/state.h>
#include <MazeBuilder/stringify_create_state.h>
#include <MazeBuilder/wavefront_object_create_state.h>

#include <fmt/format.h>

#include <algorithm>
#include <any>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

using namespace mazes;

// =============================================================================
// runtime_app constructor / destructor
// =============================================================================
runtime_app::runtime_app()
    : args_mapper{}, grid_mapper{}, processed_text_mapper{}, rng{},
      logger{
          [](async_logger::sink_type sink)
          {
              return sink;
          }(std::function<void(std::string_view)>{
              [this](std::string_view msg)
              {
                  if (msg.empty())
                  {
                      return;
                  }

                  std::lock_guard lock(logging_mtx);
                  received_logs.emplace_back(msg);
              }})},
    logging_mtx{}, received_logs{}, last_result_buffer{}, runtime_stack_ptr{std::make_unique<runtime_stack>(
                                          context{}.with_args_manager(args_mapper).with_grid_manager(grid_mapper).with_text_manager(processed_text_mapper).with_rng(rng))}
{

    register_states();

    runtime_stack_ptr->push_state(state::ID::LOADING);
    (void)visit_states();
}

runtime_app::~runtime_app() = default;

void runtime_app::register_states() const noexcept
{
    // Begin with loading_state
    runtime_stack_ptr->register_state<loading_state>(state::ID::LOADING);
    runtime_stack_ptr->register_state<parsing_state>(state::ID::PARSING);
    runtime_stack_ptr->register_state<bt_maze_create_state>(state::ID::BINARY_TREE);
    runtime_stack_ptr->register_state<dfs_maze_create_state>(state::ID::DFS);
    runtime_stack_ptr->register_state<sw_maze_create_state>(state::ID::SIDEWINDER);
    runtime_stack_ptr->register_state<pixels_create_state>(state::ID::PIXELIZING);
    runtime_stack_ptr->register_state<stringify_create_state>(state::ID::STRINGIFYING);
    runtime_stack_ptr->register_state<wavefront_object_create_state>(state::ID::WAVEFRONT_OBJECTIFY);
}

std::string_view runtime_app::apply(const std::string_view unformatted_args) noexcept
{
    try
    {
        if (auto &txt = processed_text_mapper.get(processed_text_identifier::UNKNOWN); !txt.is_processed())
        {
            txt.set(unformatted_args);

            auto sv = visit_states(unformatted_args);

            // Clear so the next apply() call can supply fresh input.
            txt.set_processed(std::monostate{});

            if (!sv.empty())
            {
                processed_text_mapper.get(processed_text_identifier::FINISHED).set_processed(sv);
                // Only return on e2e success
                return sv;
            }
        }
    }
    catch (...)
    {
        return visit_states(unformatted_args);
    }

    return {};
}

// Empty string_view on default
std::string_view runtime_app::visit_states(std::string_view sv) noexcept
{
    last_result_buffer.clear();

    {
        std::lock_guard<std::mutex> lock(logging_mtx);
        received_logs.clear();
    }

    if (!sv.empty())
    {
        runtime_stack_ptr->push_state(state::ID::PARSING);

        if (auto &txt = processed_text_mapper.get(processed_text_identifier::UNKNOWN); !txt.is_processed())
        {
            txt.set(sv);
        }
    }

    double accumulator{0.0};
    // Drive the state machine until the stack is empty
    while (!runtime_stack_ptr->is_empty())
    {
        const auto diff = progress<>::duration([&accumulator, this]
                                               { return runtime_stack_ptr->visit_states(accumulator); });
        accumulator += progress<>::to_double_from_duration(diff);
    }

#if defined(MAZE_DEBUG)

    logger.log_message(fmt::format("runtime visited states in: {:.6f} ms\n", accumulator));
#endif

    accumulator = 0.0;

    try
    {
        if (auto &txt = processed_text_mapper.get(processed_text_identifier::PROCESSING); txt.is_processed())
        {
            if (const auto processed = txt.get(); !std::holds_alternative<std::monostate>(processed))
            {
                last_result_buffer = processed_text::to_string(processed);
                return last_result_buffer;
            }
        }
    }
    catch (const std::exception &e)
    {
        logger.log_message(fmt::format("runtime - visit_states - Exception: {}", e.what()));
    }
    catch (...)
    {
        logger.log("No processed text found for identifier PROCESSING.");
    }

    return {};
}

std::string_view runtime_app::get_finished_text() noexcept
{
    last_result_buffer.clear();

    auto &txt = processed_text_mapper.get(processed_text_identifier::FINISHED);
    if (txt.is_processed())
    {
        if (const auto processed = txt.get(); !std::holds_alternative<std::monostate>(processed))
        {
            last_result_buffer = processed_text::to_string(processed);
            return last_result_buffer;
        }
    }

    return {};
}
