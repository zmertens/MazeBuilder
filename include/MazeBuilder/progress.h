#ifndef PROGRESS_H
#define PROGRESS_H

#include <chrono>
#include <mutex>

namespace mazes {

/// @brief Simple progress clock for elapsed events
/// @details This class is used to track the elapsed time between two events
/// @details Thread safe with mutexes
class progress {
    mutable std::mutex mtx;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
public:
    explicit progress();

    void reset() noexcept;

    double elapsed_s() const noexcept;

    double elapsed_ms() const noexcept;
}; // progress
} // namespace mazes

#endif // PROGRESS_H