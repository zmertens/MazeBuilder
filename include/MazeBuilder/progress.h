#ifndef PROGRESS_H
#define PROGRESS_H

#include <chrono>
#include <mutex>
#include <utility>
#include <type_traits>

/// @file progress.h
/// @namespace mazes
namespace mazes
{

    /// @class progress
    /// @brief Simple clock for elapsed events
    /// @details This class is used to track the elapsed time between two events
    /// @details Thread safe with mutexes
    template <typename Time = std::chrono::milliseconds, typename Clock = std::chrono::high_resolution_clock>
    class progress
    {
        mutable std::mutex mtx;
        Clock::time_point start_time;
        Clock::time_point end_time;

    public:
        explicit progress() : start_time(Clock::now()), end_time(start_time)
        {
        }

        /// @brief Measures the duration of a callable object
        /// @tparam F The type of the callable object
        /// @tparam ...Args The types of the arguments to the callable object
        /// @tparam Duration The type of the duration to return
        /// @param f The callable object
        /// @param args
        /// @param ...args The arguments to pass to the callable object
        /// @return The duration of the callable object
        template <typename F, typename... Args, typename Duration = Time>
        static Duration duration(F &&f, Args &&...args)
        {
            progress p;
            p.start();

            if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>)
            {
                std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
            }
            else
            {
                auto result = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
                if (!result)
                {
                    return Duration::zero();
                }
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

        static double to_double_from_duration(const std::chrono::duration<double> &duration) noexcept
        {
            return duration.count();
        }
    }; // progress

} // namespace mazes

#endif // PROGRESS_H
