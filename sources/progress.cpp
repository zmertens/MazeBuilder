#include <MazeBuilder/progress.h>

using namespace mazes;

template <typename Time, typename Clock>
void progress<Time, Clock>::start() noexcept {
    this->mtx.lock();
    start_time = Clock::now();
    this->mtx.unlock();
}

template <typename Time, typename Clock>
void progress<Time, Clock>::reset() noexcept {
    this->mtx.lock();
    start_time = end_time = typename Clock::time_point::min();
    this->mtx.unlock();
}

template <typename Time, typename Clock>
double progress<Time, Clock>::elapsed_s() const noexcept {
    std::lock_guard<std::mutex> lock(this->mtx);
    return std::chrono::duration<double>(end_time - start_time).count();
}

template <typename Time, typename Clock>
double progress<Time, Clock>::elapsed_ms() const noexcept {
    return this->elapsed_s() * 1000.0;
}
