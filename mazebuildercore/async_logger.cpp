#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/singleton_base.h>

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>
#include <utility>

#include <fmt/format.h>

using namespace mazes;

namespace
{
    void default_sink(std::string_view message)
    {
        fmt::print("{}\n", message);
    }
}

std::string mazes::format_placeholders(std::string_view format, std::vector<std::string> args)
{
    std::string result;
    result.reserve(format.size());

    std::size_t arg_index = 0;

    for (std::size_t i = 0; i < format.size(); )
    {
        if (format[i] == '{' && i + 1 < format.size() && format[i + 1] == '}')
        {
            if (arg_index < args.size())
            {
                result += args[arg_index++];
            }

            i += 2;
        } else
        {
            result += format[i];
            ++i;
        }
    }

    return result;
}

class async_logger::impl final
{
public:
    explicit impl(sink_type sink)
        : msg_sink(std::move(sink))
    {
#if !defined(__EMSCRIPTEN__)
        worker = std::jthread(
            [this](std::stop_token stop)
            {
                run(stop);
            });
#endif
    }

    ~impl()
    {
        stop();
    }

    void log(std::string message)
    {
#if defined(__EMSCRIPTEN__)

        sink_type sink;

        {
            std::lock_guard lock(mutex);
            sink = msg_sink;
        }

        if (sink)
        {
            sink(message);
        }

#else

        {
            std::lock_guard lock(mutex);

            if (stopped)
            {
                return;
            }

            queue.push_back(std::move(message));
        }

        condition.notify_one();

#endif
    }

    void flush()
    {
#if defined(__EMSCRIPTEN__)

        return;

#else

        std::unique_lock lock(mutex);

        drained.wait(lock, [this]
            { return queue.empty() && active == 0; });

#endif
    }

    void stop() noexcept
    {
#if !defined(__EMSCRIPTEN__)

        {
            std::lock_guard lock(mutex);

            if (stopped)
            {
                return;
            }

            stopped = true;
        }

    worker.request_stop();
    condition.notify_one();

#endif
    }

    void set_sink(sink_type sink)
    {
        std::lock_guard lock(mutex);
        msg_sink = std::move(sink);
    }

private:
#if !defined(__EMSCRIPTEN__)

    void run(std::stop_token stop)
    {
        while (true)
        {
            std::string message;
            sink_type sink;

            {
                std::unique_lock lock(mutex);

                condition.wait(lock, [this, &stop]
                    { return stop.stop_requested() || !queue.empty(); });

                // Finish queued messages before stopping.
                if (stop.stop_requested() && queue.empty())
                {
                    break;
                }

                message = std::move(queue.front());
                queue.pop_front();

                ++active;
                sink = msg_sink;
            }

            try
            {
                if (sink)
                {
                    sink(message);
                }
            } catch (...)
            {
                // Logging must never terminate the application.
            }

            {
                std::lock_guard lock(mutex);

                --active;

                if (queue.empty() && active == 0)
                {
                    drained.notify_all();
                }
            }
        }

        {
            std::lock_guard lock(mutex);
            drained.notify_all();
        }
    }

#endif

    mutable std::mutex mutex;

    sink_type msg_sink;

#if !defined(__EMSCRIPTEN__)

    std::condition_variable condition;
    std::condition_variable drained;

    std::deque<std::string> queue;
    std::jthread worker;

    std::size_t active{ 0 };
    bool stopped{ false };

#endif
};

async_logger::async_logger()
    : pimpl_(std::make_unique<impl>(default_sink))
{
}

async_logger::async_logger(sink_type sink)
    : pimpl_(std::make_unique<impl>(std::move(sink)))
{
}

// Defined here (not defaulted in the header) since impl must be complete for unique_ptr's deleter.
async_logger::~async_logger() = default;

void async_logger::log(std::string message)
{
    pimpl_->log(std::move(message));
}

void async_logger::log_message(std::string message)
{
    log(std::move(message));
}

void async_logger::flush()
{
    pimpl_->flush();
}

void async_logger::stop() noexcept
{
    pimpl_->stop();
}

void async_logger::set_sink(sink_type sink)
{
    pimpl_->set_sink(std::move(sink));
}

mazes::async_logger& mazes::global_async_logger()
{
    return *singleton_base<async_logger>::instance();
}

void mazes::set_printer_sink(async_logger::sink_type sink)
{
    auto& logger = global_async_logger();

    logger.flush();
    logger.set_sink(std::move(sink));
}

void mazes::reset_printer_sink()
{
    auto& logger = global_async_logger();

    logger.flush();

    logger.set_sink(default_sink);
}

void mazes::flush_printer()
{
    global_async_logger().flush();
}
