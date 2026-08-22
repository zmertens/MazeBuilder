#ifndef ASYNC_LOGGER_H
#define ASYNC_LOGGER_H

#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace mazes
{
    namespace detail
    {
        // Stringify one log/printer argument without depending on fmt in this header.
        template <typename T>
        std::string stringify_arg(const T& value)
        {
            if constexpr (std::is_convertible_v<T, std::string_view>)
            {
                return std::string(std::string_view(value));
            } else
            {
                std::ostringstream oss;
                oss << value;
                return oss.str();
            }
        }
    } // namespace detail

    /// Replace each "{}" placeholder in @p format with the matching entry from @p args, in order.
    std::string format_placeholders(std::string_view format, std::vector<std::string> args);

    class async_logger final
    {
    public:
        using sink_type = std::function<void(std::string)>;

        async_logger();
        explicit async_logger(sink_type sink);

        ~async_logger();

        async_logger(const async_logger&) = delete;
        async_logger& operator=(const async_logger&) = delete;

        async_logger(async_logger&&) = delete;
        async_logger& operator=(async_logger&&) = delete;

        /// Queue a message for asynchronous logging.
        void log(std::string message);

        /// Queue a pre-formatted message (alias for log(std::string)).
        void log_message(std::string message);

        /// Queue a message, substituting each "{}" in @p format with the corresponding argument.
        template <typename... Args>
            requires (sizeof...(Args) >= 1)
        void log(std::string_view format, const Args&... args)
        {
            log(format_placeholders(format, { detail::stringify_arg(args)... }));
        }

        /// Flush all pending messages.
        void flush();

        /// Stop the logger.
        void stop() noexcept;

        /// Replace the message sink.
        void set_sink(sink_type sink);

    private:
        class impl;
        std::unique_ptr<impl> pimpl_;
    };

    /// Get the global logger instance, lazily created via singleton_base.
    async_logger& global_async_logger();

    /// Set the global logger's sink.
    void set_printer_sink(async_logger::sink_type sink);

    /// Reset the global logger to the default stdout sink.
    void reset_printer_sink();

    /// Flush the global logger.
    void flush_printer();
} // namespace mazes

#endif // ASYNC_LOGGER_H
