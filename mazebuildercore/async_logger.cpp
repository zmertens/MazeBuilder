#include <MazeBuilder/async_logger.h>

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include <fmt/format.h>

using namespace mazes;

namespace
{
    void default_sink(std::string_view message)
    {
        fmt::println("{}", message);
    }
}

class async_logger::impl
{
public:
    explicit impl(sink_type sink)
        : msg_sink(std::move(sink))
    {
#if !defined(__EMSCRIPTEN__)
        worker = std::thread(
            [this]()
            {
                run();
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
        {
            std::lock_guard lock(mutex);

            if (stopped)
            {
                return;
            }

            stopped = true;
        }

        if (worker.joinable())
        {
            condition.notify_all();
            worker.join();
        }
    }

    void set_sink(sink_type sink)
    {
        std::lock_guard lock(mutex);
        msg_sink = std::move(sink);
    }

private:
    void run()
    {
        while (true)
        {
            std::string message;
            sink_type sink;

            {
                std::unique_lock lock(mutex);

                condition.wait(lock, [this]
                               { return stopped || !queue.empty(); });

                // Finish queued messages before stopping.
                if (queue.empty())
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
            }
            catch (...)
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

    mutable std::mutex mutex;

    sink_type msg_sink;

    std::condition_variable condition;
    std::condition_variable drained;

    std::deque<std::string> queue;
    std::thread worker;

    std::size_t active{0};
    bool stopped{false};
};

async_logger::async_logger()
    : pimpl_(std::make_unique<impl>(default_sink))
{
}

// Defined here (not defaulted in the header) since impl must be complete for unique_ptr's deleter.
async_logger::~async_logger() = default;

void async_logger::log(std::string_view message)
{
    pimpl_->log(std::string{message});
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
