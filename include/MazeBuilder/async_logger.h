#ifndef ASYNC_LOGGER_H
#define ASYNC_LOGGER_H

#include <functional>
#include <memory>
#include <string_view>
#include <utility>

#include <MazeBuilder/singleton_base.h>

namespace mazes
{

    class async_logger final : public singleton_base<async_logger>
    {
        friend class singleton_base<async_logger>;
    public:
        using sink_type = std::function<void(std::string_view)>;

        async_logger();
        ~async_logger();

        /// Queue a message for asynchronous logging.
        void log(std::string_view message);

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
} // namespace mazes

#endif // ASYNC_LOGGER_H
