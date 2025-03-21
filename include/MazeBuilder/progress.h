#ifndef PROGRESS_H
#define PROGRESS_H

#include <chrono>
#include <mutex>
#include <utility>

namespace mazes {

/// @file progress.h
/// @class progress
/// @brief Simple clock for elapsed events
/// @details This class is used to track the elapsed time between two events
/// @details Thread safe with mutexes
template <typename Time = std::chrono::microseconds, typename Clock = std::chrono::high_resolution_clock>
class progress {
    mutable std::mutex mtx;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
public:

    /// @brief 
    /// @tparam F 
    /// @tparam ...Args 
    /// @tparam Duration 
    /// @param f 
    /// @param ...args 
    /// @return 
    template <typename F, typename... Args, typename Duration = Time>
    static Duration duration(F&& f, Args&&... args) {
        auto start = Clock::now();

        auto result = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);

        if (!result) {
            return Duration::zero();
        }

        auto end = Clock::now();
        auto duration = std::chrono::duration_cast<Duration>(end - start);
        return duration;
    }


    void start() noexcept;

    /// @brief Reset the clock
    void reset() noexcept;

    /// @brief Get the elapsed time in seconds
    /// @return 
    double elapsed_s() const noexcept;

    /// @brief Get the elapsed time in milliseconds
    double elapsed_ms() const noexcept;
}; // progress
} // namespace mazes

#endif // PROGRESS_H
