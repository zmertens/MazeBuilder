#include <MazeBuilder/runtime_app.h>

#include <MazeBuilder/bt_maze_create_state.h>
#include <MazeBuilder/dfs_maze_create_state.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/loading_state.h>
#include <MazeBuilder/parsing_state.h>
#include <MazeBuilder/pixels_create_state.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_stack.h>
#include <MazeBuilder/sw_maze_create_state.h>
#include <MazeBuilder/state.h>
#include <MazeBuilder/stringify_create_state.h>
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
    : m_args_mapper{}, m_grid_mapper{}, m_processed_text_mapper{}, m_rng{},
      m_logger{
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

                  std::lock_guard lock(m_logging_mtx);
                  m_received_logs.emplace_back(msg);
              }})},
      m_logging_mtx{}, m_received_logs{}, runtime_stack_ptr{std::make_unique<runtime_stack>(
                                              context{}.with_args_manager(m_args_mapper).with_grid_manager(m_grid_mapper).with_text_manager(m_processed_text_mapper).with_rng(m_rng).with_raw_input(m_current_input))}
{
    try
    {
        m_args_mapper.load<args>(args_identifier::COMMON);
    }
    catch (const std::exception &e)
    {
        m_logger.log("Failed to load arguments during runtime: {}\n", e.what());
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

std::string_view runtime_app::apply(const std::string_view unformatted_sv) noexcept
{
    m_current_input = std::string{unformatted_sv};
    m_last_result.clear();
    m_last_grid_id = grid_identifier::BASIC;

    auto &a = m_args_mapper.get(args_identifier::COMMON);
    const bool has_program_name = unformatted_sv.find_first_of("-") != 0;

    if (!a.parse(std::cref(m_current_input), has_program_name))
    {
        return {};
    }

    if (const auto parsed = a.get(); parsed.has_value())
    {
        if (parsed->contains(args::DISTANCES_WORD_STR))
        {
            m_last_grid_id = grid_identifier::DISTANCE;
        }

        return visit_states(std::cref(a));
    }

    return {};
}

std::string_view runtime_app::visit_states(const std::optional<args> &arguments) noexcept
{
    {
        std::lock_guard<std::mutex> lock(m_logging_mtx);
        m_received_logs.clear();
    }

    // If the stack drained (e.g. a prior call exhausted it), re-seed the state machine.
    // Run LOADING only when core resources are not initialized yet; otherwise jump
    // directly to PARSING to avoid reloading heavy resources on every apply().
    if (runtime_stack_ptr->is_empty())
    {
        if (m_grid_mapper.is_empty() || m_processed_text_mapper.is_empty())
        {
            runtime_stack_ptr->push_state(state::ID::LOADING);
        }
        else
        {
            runtime_stack_ptr->push_state(state::ID::PARSING);
        }
    }

    // Drive the state machine until the stack is empty
    while (!runtime_stack_ptr->is_empty())
    {
        runtime_stack_ptr->visit_states(arguments, 0.0);
    }

    try
    {
        const auto &txt = m_processed_text_mapper.get(processed_text_identifier::FINISHED);
        if (const auto processed = txt.get_processed(); !std::holds_alternative<std::monostate>(processed))
        {
            if (const auto *str = std::get_if<std::string>(&processed))
            {
                m_last_result = *str;
            }
            else if (const auto *sv = std::get_if<std::string_view>(&processed))
            {
                m_last_result = std::string{*sv};
            }
            else if (const auto *cstr = std::get_if<char *>(&processed); cstr && *cstr)
            {
                m_last_result = *cstr;
            }
        }
    }
    catch (const std::out_of_range &)
    {
        m_logger.flush();
        return {};
    }

#if defined(MAZE_DEBUG)
    {
        std::lock_guard<std::mutex> lock(m_logging_mtx);
        std::ranges::for_each(m_received_logs, [](const auto &log)
                              {
            if (!log.empty())
            {
                fmt::print("Log: {}\n", log);
            } });
    }
#endif

    m_logger.flush();
    return m_last_result;
}

grid_interface *runtime_app::get_last_grid() noexcept
{
    try
    {
        return &m_grid_mapper.get(m_last_grid_id);
    }
    catch (...)
    {
        return nullptr;
    }
}
