#include <MazeBuilder/progress.h>

using namespace mazes;

progress::progress() 
: start_time(std::chrono::steady_clock::now())
, end_time(std::chrono::steady_clock::now()) {

}

void progress::reset() noexcept {
    this->mtx.lock();
    start_time = end_time = std::chrono::steady_clock::now();
    this->mtx.unlock();
}

double progress::elapsed_s() const noexcept {
    std::lock_guard<std::mutex> lock(this->mtx);
    return std::chrono::duration<double>(end_time - start_time).count();
}

double progress::elapsed_ms() const noexcept {
    return this->elapsed_s() * 1000.0;
}