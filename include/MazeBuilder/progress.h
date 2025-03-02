#ifndef PROGRESS_H
#define PROGRESS_H

#include <chrono>
#include <mutex>

namespace mazes {

/// @file progress.h
/// @class progress
/// @brief Simple clock for elapsed events
/// @details This class is used to track the elapsed time between two events
/// @details Thread safe with mutexes
class progress {
    mutable std::mutex mtx;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
public:
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
