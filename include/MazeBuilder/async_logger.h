#ifndef ASYNC_LOGGER_H
#define ASYNC_LOGGER_H

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

/// @namespace mazes
/// @file async_logger.h
namespace mazes
{
    /// @brief Asynchronous logger class
    class async_logger final
    {
    public:
        /// @brief Type alias for the sink function
        using sink_type = std::function<void(std::string_view)>;

        async_logger()
            : sink_([](std::string_view msg)
              {
                  fmt::print("{}\n", msg);
              }),
              worker_([this](const std::stop_token& st)
              {
                  run(st);
              })
        {
        }

        explicit async_logger(sink_type sink)
            : sink_(std::move(sink)), worker_([this](const std::stop_token& st)
            {
                run(st);
            })
        {
        }

        ~async_logger()
        {
            stop();
        }

        async_logger(const async_logger&) = delete;
        async_logger& operator=(const async_logger&) = delete;
        async_logger(async_logger&&) = delete;
        async_logger& operator=(async_logger&&) = delete;

        /// @brief Log a formatted message
        /// @tparam ...FormatArgs
        /// @param fmt_s Format string
        /// @param ...args Arguments for the format string
        template <typename... FormatArgs>
        void log(fmt::format_string<FormatArgs...> fmt_s, FormatArgs&&... args)
        {
            enqueue(fmt::format(fmt_s, std::forward<FormatArgs>(args)...));
        }

        /// @brief Log a message
        /// @param msg Message to log
        void log_message(std::string msg)
        {
            enqueue(std::move(msg));
        }

        /// @brief Flush the logger, blocking until all pending messages are processed
        void flush()
        {
            std::unique_lock lock(mtx_);
            drained_cv_.wait(lock, [this]()
            {
                return pending_ == 0;
            });
        }

        /// @brief Stop the logger, blocking until all pending messages are processed
        void stop() noexcept
        {
            bool should_join = false;
            {
                std::lock_guard lock(mtx_);
                if (!stopped_)
                {
                    stopped_ = true;
                    should_join = true;
                }
            }

            if (should_join)
            {
                worker_.request_stop();
                cv_.notify_all();
                if (worker_.joinable())
                {
                    worker_.join();
                }
            }
        }

        /// @brief Set the sink function for the logger
        /// @param sink Sink function to handle log messages
        void set_sink(sink_type sink)
        {
            std::lock_guard lock(mtx_);
            sink_ = std::move(sink);
        }

    private:
        /// @brief Enqueue a message for logging
        /// @param msg Message to enqueue
        void enqueue(std::string msg)
        {
            std::lock_guard lock(mtx_);
            if (stopped_)
            {
                return;
            }

            queue_.emplace_back(std::move(msg));
            ++pending_;
            cv_.notify_one();
        }

        /// @brief Run the logger, processing messages until stopped
        /// @param st Stop token to request stopping the logger
        void run(const std::stop_token& st)
        {
            for (;;)
            {
                std::string message;
                sink_type sink;

                {
                    std::unique_lock lock(mtx_);
                    cv_.wait(lock, [this, &st]()
                    {
                        return st.stop_requested() || !queue_.empty();
                    });

                    if (st.stop_requested() && queue_.empty())
                    {
                        break;
                    }

                    message = std::move(queue_.front());
                    queue_.pop_front();
                    sink = sink_;
                }

                try
                {
                    if (sink)
                    {
                        sink(message);
                    }
                }
                catch (...)
                {
                }

                {
                    std::lock_guard lock(mtx_);
                    if (pending_ > 0)
                    {
                        --pending_;
                    }
                    if (pending_ == 0)
                    {
                        drained_cv_.notify_all();
                    }
                }
            }

            std::lock_guard lock(mtx_);
            if (pending_ == 0)
            {
                drained_cv_.notify_all();
            }
        }

        mutable std::mutex mtx_;
        std::condition_variable cv_;
        std::condition_variable drained_cv_;
        std::deque<std::string> queue_;
        sink_type sink_;
        std::jthread worker_;
        std::size_t pending_{0};
        bool stopped_{false};
    };

    /// @brief Get the global async logger instance
    /// @return Reference to the global async logger
    inline async_logger& global_async_logger()
    {
        static async_logger logger{};
        return logger;
    }

    /// @brief Set the sink function for the global async logger
    /// @param sink Sink function to handle log messages
    inline void set_printer_sink(async_logger::sink_type sink)
    {
        // Drain already-queued messages to the current sink before rerouting output.
        global_async_logger().flush();
        global_async_logger().set_sink(std::move(sink));
    }

    /// @brief Flush the global async logger, blocking until all pending messages are processed
    inline void reset_printer_sink()
    {
        // Preserve message ordering and avoid redirecting queued messages mid-flight.
        global_async_logger().flush();
        global_async_logger().set_sink([](std::string_view msg)
        {
            fmt::print("{}\n", msg);
        });
    }

    /// @brief Flush the global async logger, blocking until all pending messages are processed
    inline void flush_printer()
    {
        global_async_logger().flush();
    }

    /// @brief Log a message using the global async logger
    /// @tparam ...Args Types of the arguments to format
    /// @param ...args Arguments to format and log
    template <typename... Args>
    void printer(Args&&... args)
    {
        std::vector<std::string> parts = {fmt::format("{}", std::forward<Args>(args))...};
        global_async_logger().log("{}", fmt::join(parts, ", "));
    }
} // namespace mazes

#endif // ASYNC_LOGGER_H
