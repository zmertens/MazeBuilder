// State machine workflow: (load once) -> parse -> generate via algo -> output

#include <MazeBuilder/runtime_app.h>

#include <MazeBuilder/bt_maze_create_state.h>
#include <MazeBuilder/dfs_maze_create_state.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/loading_state.h>
#include <MazeBuilder/masked_maze_and_create_state.h>
#include <MazeBuilder/parsing_state.h>
#include <MazeBuilder/pixels_create_state.h>
#include <MazeBuilder/prims_maze_create_state.h>
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

runtime_app::runtime_app()
    : args_mapper{}
    , grid_mapper{}
    , processed_text_mapper{}
    , rng{}
    , logging_mtx{}
    , received_logs{}
    , last_result_buffer{}
    , runtime_stack_ptr{ std::make_unique<runtime_stack>(context{}
        .with_args_manager(args_mapper)
        .with_grid_manager(grid_mapper)
        .with_text_manager(processed_text_mapper)
        .with_rng(rng)) }
{
    global_async_logger().set_sink(
        [](async_logger::sink_type sink)
        {
            return sink;
        }(std::function<void(std::string)>{
            [this](const auto msg)
                {
                    if (msg.empty())
                    {
                        return;
                    }

                    std::lock_guard lock(logging_mtx);
                    received_logs.emplace_back(msg);
                }}));

            register_states();
            runtime_stack_ptr->push_state(state::ID::LOAD);
            runtime_stack_ptr->visit_states(0.0);
}

runtime_app::~runtime_app() = default;

void runtime_app::register_states() const noexcept
{
    // Begin with loading_state
    runtime_stack_ptr->register_state<loading_state>(state::ID::LOAD);
    runtime_stack_ptr->register_state<parsing_state>(state::ID::PARSE);
    runtime_stack_ptr->register_state<bt_maze_create_state>(state::ID::LINK_WITH_BINARY_TREE);
    runtime_stack_ptr->register_state<dfs_maze_create_state>(state::ID::LINK_WITH_DFS);
    runtime_stack_ptr->register_state<sw_maze_create_state>(state::ID::LINK_WITH_SIDEWINDER);
    runtime_stack_ptr->register_state<prims_maze_create_state>(state::ID::LINK_WITH_PRIMS);
    runtime_stack_ptr->register_state<masked_maze_and_create_state>(state::ID::LINK_WITH_MASKED);
    runtime_stack_ptr->register_state<pixels_create_state>(state::ID::WRITE_TO_IMAGE);
    runtime_stack_ptr->register_state<stringify_create_state>(state::ID::WRITE_TO_STRING);
    runtime_stack_ptr->register_state<wavefront_object_create_state>(state::ID::WRITE_TO_WF_OBJ);
}

std::string_view runtime_app::apply(const std::string_view unformatted_args) noexcept
{
#if defined(MAZE_DEBUG)
    global_async_logger().log(fmt::format("runtime received unformatted args: {}\n", unformatted_args));
#endif

    try
    {
        if (auto& unknown_txt = processed_text_mapper.get(processed_text_identifier::UNKNOWN); !unknown_txt.is_processed())
        {
            unknown_txt.set(unformatted_args);

            runtime_stack_ptr->push_state(state::ID::PARSE);

            visit_states();

            // Clear so the next apply() call can supply fresh input.
            unknown_txt.set_processed(std::monostate{});

            last_result_buffer = get_finished_text();
            return last_result_buffer;
        }
    } catch (...)
    {
        global_async_logger().log("No processed text found.");
    }

    return {};
}

// Empty string on default
void runtime_app::visit_states() noexcept
{
    last_result_buffer.clear();

    {
        std::lock_guard<std::mutex> lock(logging_mtx);
        received_logs.clear();
    }

    double accumulator{ 0.0 };
    // Drive the state machine until the stack is empty
    while (!runtime_stack_ptr->is_empty())
    {
        const auto diff = progress<>::duration([&accumulator, this]
            { return runtime_stack_ptr->visit_states(accumulator); });
        accumulator += progress<>::to_double_from_duration(diff);
    }

#if defined(MAZE_DEBUG)

    global_async_logger().log(fmt::format("runtime visited states in: {:.6f} ms\n", accumulator));
    global_async_logger().log(fmt::format("runtime finished text:\n{}", get_finished_text()));
#endif
}

std::string runtime_app::get_finished_text() noexcept
{
    if (const auto& txt{ processed_text_mapper.get(processed_text_identifier::FINISHED) }; txt.is_processed()) {
        return processed_text::to_string(txt.get());
    }

    return {};
}
