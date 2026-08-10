#ifndef RUNTIME_APP_H
#define RUNTIME_APP_H

#include <MazeBuilder/args.h>
#include <MazeBuilder/app_contract.h>
#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/processed_text.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/singleton_base.h>

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

/// @file runtime_app.h
/// @namespace mazes
namespace mazes
{
    class grid_interface;
    class runtime_stack;

    /// @brief The main application class for the maze builder, responsible for managing the runtime stack and resources
    class runtime_app final : public app_contract, public singleton_base<runtime_app>
    {
        friend class singleton_base<runtime_app>;

    public:
        /// @brief struct representing the context passed to states in the runtime_app stack
        struct context final
        {
            [[nodiscard]] args_manager *get_args_manager() const noexcept { return _args; }

            context &with_args_manager(args_manager &am) noexcept
            {
                _args = &am;
                return *this;
            }

            [[nodiscard]] grid_manager *get_grid_manager() const noexcept { return _grid_manager; }

            context &with_grid_manager(grid_manager &g) noexcept
            {
                _grid_manager = &g;
                return *this;
            }

            [[nodiscard]] processed_text_manager *get_text_manager() const noexcept
            {
                return _text_manager;
            }

            context &with_text_manager(processed_text_manager &ptm) noexcept
            {
                _text_manager = &ptm;
                return *this;
            }

            [[nodiscard]] randomizer *get_rng() const noexcept { return _rng; }

            context &with_rng(randomizer &r) noexcept
            {
                _rng = &r;
                return *this;
            }

            [[nodiscard]] const std::string *get_raw_input() const noexcept { return _raw_input; }

            context &with_raw_input(const std::string &raw_input) noexcept
            {
                _raw_input = &raw_input;
                return *this;
            }

        private:
            args_manager *_args{};
            grid_manager *_grid_manager{};
            processed_text_manager *_text_manager{};
            randomizer *_rng{};
            const std::string *_raw_input{};
        };

        // Constructor / Destructor
        explicit runtime_app();
        ~runtime_app() override;

        /// @brief Applies the given unformatted string view to the runtime application
        /// @param unformatted_sv The unformatted string view to be processed
        /// @return A string view representing the result of the application
        [[nodiscard]] std::string_view apply(std::string_view unformatted_sv) noexcept override;

        /// @brief Returns the last generated grid used by apply(), if available.
        [[nodiscard]] grid_interface *get_last_grid() noexcept;

    private:
        /// @brief Registers the states for the runtime stack, associating state IDs with their corresponding factories
        void register_states() const noexcept;

        /// @brief Iterate over the stack (top-to-bottom), call update(args) on each state
        ///        until one returns false, apply pending changes, then extract the
        ///        first ready result from processed_text_mapper.
        /// @param arguments The optional arguments to pass to each state's update function
        /// @return A string view representing the result of the state updates
        [[nodiscard]] std::string_view visit_states(const std::optional<args> &arguments) noexcept;

        args_manager m_args_mapper;
        grid_manager m_grid_mapper;
        processed_text_manager m_processed_text_mapper;
        randomizer m_rng;

        async_logger m_logger;
        std::mutex m_logging_mtx;
        std::vector<std::string> m_received_logs;

        std::string m_last_result;
        std::string m_current_input;
        grid_identifier m_last_grid_id{grid_identifier::BASIC};
        std::unique_ptr<runtime_stack> runtime_stack_ptr;
    };
} // namespace mazes

#endif // RUNTIME_APP_H
