#ifndef PROGRESS_H
#define PROGRESS_H

#include <chrono>
#include <mutex>
#include <utility>

namespace mazes
{

    /// @file progress.h
    /// @class progress
    /// @brief Simple clock for elapsed events
    /// @details This class is used to track the elapsed time between two events
    /// @details Thread safe with mutexes
    template <typename Time = std::chrono::microseconds, typename Clock = std::chrono::high_resolution_clock>
    class progress
    {
        mutable std::mutex mtx;
        typename Clock::time_point start_time;
        typename Clock::time_point end_time;

    public:
        explicit progress() : start_time(Clock::now()), end_time(start_time) {}

        /// @brief
        /// @tparam F
        /// @tparam ...Args
        /// @tparam Duration
        /// @param f
        /// @param ...args
        /// @return
        template <typename F, typename... Args, typename Duration = Time>
        static Duration duration(F &&f, Args &&...args)
        {
            progress p;
            p.start();

            auto result = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);

            if (!result)
            {
                return Duration::zero();
            }

            auto duration = p.elapsed<Duration>();
            return duration;
        }

        /// @brief Start the progress
        void start() noexcept
        {
            std::lock_guard<std::mutex> lock(this->mtx);
            start_time = end_time = Clock::now();
        }

        /// @brief Reset the progress
        void reset() noexcept
        {
            this->start();
        }

        /// @brief Capture the elapsed time
        /// @tparam T
        /// @return
        template <typename T = double>
        T elapsed() noexcept
        {
            std::lock_guard<std::mutex> lock(this->mtx);
            end_time = Clock::now();
            return static_cast<T>(std::chrono::duration_cast<Time>(end_time - start_time).count());
        }
    }; // progress
} // namespace mazes

#endif // PROGRESS_H
