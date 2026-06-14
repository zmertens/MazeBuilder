#include <MazeBuilder/runtime_app.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/bt_maze_create_state.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/dfs_maze_create_state.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/loading_state.h>
#include <MazeBuilder/objectify_create_state.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/progress.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/sw_maze_create_state.h>
#include <MazeBuilder/state.h>
#include <MazeBuilder/stringify_create_state.h>
#include <MazeBuilder/parsing_state.h>
#include <MazeBuilder/wavefront_object_create_state.h>

#include <fmt/format.h>

#include <chrono>
#include <array>
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
    : grid_mapper{}, processed_text_mapper{}, rng{}, args_mapper{},
      runtime_context{
          context().with_args_manager(args_mapper).with_grid_manager(grid_mapper).with_text_manager(processed_text_mapper).with_rng(rng).with_raw_input(m_current_input)},
      runtime_stack_ptr{std::make_unique<runtime_stack>(runtime_context)}, logging_mtx{}, logging_cv{}, received_logs{},
      logger{[](async_logger::sink_type sink)
             { return sink; }(std::function<void(std::string_view)>{[this](std::string_view msg)
                                                                    {
                            if (msg.empty())
                            {
                                return;
                            }

                            std::lock_guard<std::mutex> lock(logging_mtx);
                            received_logs.emplace_back(std::string{msg});
                            logging_cv.notify_one(); }})}
{
    try
    {
        args_mapper.load<args>(args_identifier::COMMON);
    }
    catch (const std::exception &e)
    {
        logger.log("Failed to load arguments during runtime: {}\n", e.what());
        // Instance is created but no states registered and cannot process input
        return;
    }

    register_states();
    runtime_stack_ptr->push_state(state::ID::LOADING);
}

runtime_app::~runtime_app() = default;

// =============================================================================
// runtime_app methods
// =============================================================================

void runtime_app::register_states() noexcept
{
    // Begin with loading_state
    runtime_stack_ptr->register_state<loading_state>(state::ID::LOADING);
    runtime_stack_ptr->register_state<parsing_state>(state::ID::PARSING);
    runtime_stack_ptr->register_state<bt_maze_create_state>(state::ID::BTING);
    runtime_stack_ptr->register_state<dfs_maze_create_state>(state::ID::DFSING);
    runtime_stack_ptr->register_state<sw_maze_create_state>(state::ID::SIDEWINDERING);
    runtime_stack_ptr->register_state<objectify_create_state>(state::ID::OBJECTIFYING);
    runtime_stack_ptr->register_state<stringify_create_state>(state::ID::STRINGIFYING);
    runtime_stack_ptr->register_state<wavefront_object_create_state>(state::ID::WAVEFRONT_OBJECTIFYING);
}

std::string_view runtime_app::apply([[maybe_unused]] std::string_view unformatted_sv) noexcept
{
    m_current_input = std::string{unformatted_sv};

    // Parse the raw argument string into an args object and drive the state machine
    if (auto &a = args_mapper.get(args_identifier::COMMON); !unformatted_sv.empty() && a.parse(std::string{unformatted_sv}))
    {
        return visit_states(a);
    }

    return {};
}

std::string_view runtime_app::visit_states(const std::optional<args> &args) noexcept
{
    {
        std::lock_guard<std::mutex> lock(logging_mtx);
        received_logs.clear();
    }

    // If the stack drained (e.g. a prior call exhausted it), re-seed the loading state
    // so repeated apply() calls each get a fresh run through the machine.
    if (runtime_stack_ptr->is_empty())
    {
        runtime_stack_ptr->push_state(state::ID::LOADING);
    }

    // do fixed time steps here
    double last_update_time = 0.0;
    static constexpr double TIME_STEP_SECONDS = 0.1; // 100ms per step

    // Drive the state machine until the stack is empty
    while (!runtime_stack_ptr->is_empty())
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(
                             start_time - std::chrono::high_resolution_clock::time_point(
                                              std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                  std::chrono::duration<double>(last_update_time))))
                             .count();
        if (elapsed >= TIME_STEP_SECONDS)
        {
            auto visiting_duration = mazes::progress<>::duration(&runtime_stack::visit_states, runtime_stack_ptr.get(), args, elapsed);
            const double visiting_seconds = std::chrono::duration<double>(visiting_duration).count();
            logger.log("Visited states in {:.6f} seconds.", visiting_seconds);
        }
        last_update_time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_time).count();
    }

    try
    {
        auto &txt = processed_text_mapper.get(processed_text_identifier::FINISHED);
        if (!txt.is_clean())
        {
            if (auto processed = txt.get_processed(); !std::holds_alternative<std::monostate>(processed))
            {
                if (auto *str = std::get_if<std::string>(&processed))
                {
                    m_last_result = *str;
                }
            }
        }
    }
    catch (const std::out_of_range &)
    {
        logger.flush();
        return {};
    }

#if defined(MAZE_DEBUG)
    {
        std::lock_guard<std::mutex> lock(logging_mtx);
        std::ranges::for_each(received_logs, [](const auto &log)
                              {
                                  if (!log.empty())
                                  {
                                      fmt::print("Log: {}\n", log);
                                  } });
    }
#endif

    logger.flush();
    return m_last_result;
}
