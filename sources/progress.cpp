#include <MazeBuilder/progress.h>

using namespace mazes;

void progress::start() noexcept {
    this->mtx.lock();
    start_time = std::chrono::steady_clock::now();
    this->mtx.unlock();
}

void progress::reset() noexcept {
    this->mtx.lock();
    start_time = end_time = std::chrono::steady_clock::time_point::min();
    this->mtx.unlock();
}

double progress::elapsed_s() const noexcept {
    std::lock_guard<std::mutex> lock(this->mtx);
    return std::chrono::duration<double>(end_time - start_time).count();
}

double progress::elapsed_ms() const noexcept {
    return this->elapsed_s() * 1000.0;
}