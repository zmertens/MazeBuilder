#ifndef RUNTIME_APP_H
#define RUNTIME_APP_H

#include <MazeBuilder/args.h>
#include <MazeBuilder/app_contract.h>
#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/singleton_base.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/// @file runtime_app.h
/// @namespace mazes
namespace mazes
{
    class runtime_stack;

    /// @brief The main application class for the maze builder, responsible for managing the runtime stack and resources
    class runtime_app final : public app_contract, public singleton_base<runtime_app>
    {
        friend class singleton_base<runtime_app>;

    public:
        /// @brief struct representing the context passed to states in the runtime_app stack
        struct context final
        {
            [[nodiscard]] args_manager *get_args_manager() const noexcept { return _args.value_or(nullptr); }

            context &with_args_manager(args_manager &am) noexcept
            {
                _args = &am;
                return *this;
            }

            [[nodiscard]] grid_manager *get_grid_manager() const noexcept { return _grid_manager.value_or(nullptr); }

            context &with_grid_manager(grid_manager &g) noexcept
            {
                _grid_manager = &g;
                return *this;
            }

            [[nodiscard]] processed_text_manager *get_text_manager() const noexcept { return _text_manager.value_or(nullptr); }

            context &with_text_manager(processed_text_manager &ptm) noexcept
            {
                _text_manager = &ptm;
                return *this;
            }

            [[nodiscard]] std::optional<std::reference_wrapper<randomizer>> get_rng() const noexcept { return _rng; }

            context &with_rng(randomizer &r) noexcept
            {
                _rng = r;
                return *this;
            }

            [[nodiscard]] const std::string *get_raw_input() const noexcept { return _raw_input.value_or(nullptr); }

            context &with_raw_input(const std::string &raw_input) noexcept
            {
                _raw_input = &raw_input;
                return *this;
            }

        private:
            std::optional<args_manager *> _args;
            std::optional<grid_manager *> _grid_manager;
            std::optional<processed_text_manager *> _text_manager;
            std::optional<std::reference_wrapper<randomizer>> _rng;
            std::optional<const std::string *> _raw_input;
        };

        // Constructor / Destructor
        explicit runtime_app();
        ~runtime_app() override;

        /// @brief Applies the given unformatted string view to the runtime application
        /// @param unformatted_sv The unformatted string view to be processed
        /// @return A string view representing the result of the application
        [[nodiscard]] std::string_view apply(std::string_view unformatted_sv) noexcept override;

    private:
        /// @brief Registers the states for the runtime stack, associating state IDs with their corresponding factories
        void register_states() noexcept;

        /// @brief Iterate over the stack (top-to-bottom), call update(args) on each state
        ///        until one returns false, apply pending changes, then extract the
        ///        first ready result from processed_text_mapper.
        /// @param arguments The optional arguments to pass to each state's update function
        /// @return A string view representing the result of the state updates
        [[nodiscard]] std::string_view visit_states(const std::optional<args> &arguments) noexcept;

        args_manager args_mapper;
        grid_manager grid_mapper;
        processed_text_manager processed_text_mapper;
        randomizer rng;

        async_logger logger;
        std::mutex logging_mtx;
        std::condition_variable logging_cv;
        std::vector<std::string> received_logs;

        std::string m_last_result;
        std::string m_current_input;
        context runtime_context;
        std::unique_ptr<runtime_stack> runtime_stack_ptr;
    };

} // namespace mazes

#endif // RUNTIME_APP_H
