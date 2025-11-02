#include "WorkerConcurrent.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>

#include <SDL3/SDL.h>

WorkerConcurrent::WorkItem::WorkItem(const float maxTime, float elapsedTime, int start, int count)
    : maxTime(maxTime), elapsedTime(elapsedTime), start{ start }, count{ count } {

}

// Constructor
WorkerConcurrent::WorkerConcurrent()
    : gameMtx(SDL_CreateMutex()), gameCond(SDL_CreateCondition()), pendingWorkCount(0), shouldExit(false) {

    if (!gameCond) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating condition variable: %s\n", SDL_GetError());
    }

    if (!gameMtx) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating mutex: %s\n", SDL_GetError());
    }
}

// Destructor
WorkerConcurrent::~WorkerConcurrent() {

    // Signal threads to exit
    SDL_LockMutex(gameMtx);
    shouldExit = true;
    SDL_BroadcastCondition(gameCond);
    SDL_UnlockMutex(gameMtx);

    // Wait for all threads to finish
    for (auto t : threads) {
        if (t) {

            auto name = SDL_GetThreadName(t);
            int status = 0;
            SDL_WaitThread(t, &status);
#if defined(MAZE_DEBUG)

            SDL_Log("Worker thread with status [ %s | %d ] finished\n", name, status);
#endif
        }
    }

    SDL_DestroyMutex(gameMtx);
    SDL_DestroyCondition(gameCond);
}

// Copy Constructor
WorkerConcurrent::WorkerConcurrent(const WorkerConcurrent& other)
    : workQueue(other.workQueue), pendingWorkCount(other.pendingWorkCount) {

    gameMtx = SDL_CreateMutex();
    gameCond = SDL_CreateCondition();

    if (!gameMtx || !gameCond) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating SDL concurrency objects: %s\n", SDL_GetError());
    }
    // Threads are not copied as they cannot be shared between instances
}

// Copy Assignment Operator
WorkerConcurrent& WorkerConcurrent::operator=(const WorkerConcurrent& other) {
    if (this == &other) {
        return *this;
    }

    // Clean up existing resources
    for (auto thread : threads) {
        if (thread) {
            SDL_WaitThread(thread, nullptr);
        }
    }
    SDL_DestroyMutex(gameMtx);
    SDL_DestroyCondition(gameCond);

    // Copy data
    for (auto&& item : other.workQueue) {
        workQueue.push_back(item);
    }
    pendingWorkCount = other.pendingWorkCount;

    // Reinitialize mutex and condition variable
    gameMtx = SDL_CreateMutex();
    gameCond = SDL_CreateCondition();

    if (!gameMtx || !gameCond) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating SDL concurrency objects: %s\n", SDL_GetError());
    }

    return *this;
}

// Move Constructor
WorkerConcurrent::WorkerConcurrent(WorkerConcurrent&& other) noexcept
    : workQueue(std::move(other.workQueue))
    , threads(std::move(other.threads))
    , gameMtx(other.gameMtx)
    , gameCond(other.gameCond)
    , pendingWorkCount(other.pendingWorkCount) {

    other.gameMtx = nullptr;
    other.gameCond = nullptr;
}

// Move Assignment Operator
WorkerConcurrent& WorkerConcurrent::operator=(WorkerConcurrent&& other) noexcept {
    if (this == &other) {

        return *this;
    }

    // Clean up existing resources
    for (auto thread : threads) {
        if (thread) {

            SDL_WaitThread(thread, nullptr);
        }
    }

    SDL_DestroyMutex(gameMtx);
    SDL_DestroyCondition(gameCond);

    // Move data
    workQueue = std::move(other.workQueue);
    threads = std::move(other.threads);
    gameMtx = other.gameMtx;
    gameCond = other.gameCond;
    pendingWorkCount = other.pendingWorkCount;

    // Nullify other's resources
    other.gameMtx = nullptr;
    other.gameCond = nullptr;

    return *this;
}

// Initialize worker threads
void WorkerConcurrent::initThreads() noexcept {

    using std::back_inserter;
    using std::copy;
    using std::cref;
    using std::ref;
    using std::string;
    using std::to_string;
    using std::vector;

    auto threadFunc = [](void* data) -> int {

        if (auto workerPtr = reinterpret_cast<WorkerConcurrent*>(data)) {

            float elapsedTime = 0.0f;

            while (!workerPtr->shouldExit) {
                {
                    SDL_LockMutex(workerPtr->gameMtx);
                    while (workerPtr->workQueue.empty() && !workerPtr->shouldExit) {
                        SDL_WaitCondition(workerPtr->gameCond, workerPtr->gameMtx);
                    }

                    // Check if we should exit before processing
                    if (workerPtr->shouldExit) {
                        SDL_UnlockMutex(workerPtr->gameMtx);
                        break;
                    }

                    if (!workerPtr->workQueue.empty()) {

                        auto&& tempWorker = workerPtr->workQueue.front();
                        workerPtr->workQueue.pop_front();

#if defined(MAZE_DEBUG)

                        SDL_Log("Processing work item [ start: %d | count: %d]\n", tempWorker.start, tempWorker.count);
#endif

                        SDL_UnlockMutex(workerPtr->gameMtx);

                        workerPtr->doWork(ref(elapsedTime), cref(tempWorker));

                        if (elapsedTime >= tempWorker.maxTime) {
                            
                            SDL_Log("Max time reached for this work item: %.2f >= %.2f\n", elapsedTime, tempWorker.maxTime);
                            elapsedTime = 0.0f;
                        }
                    } else {
                        SDL_UnlockMutex(workerPtr->gameMtx);
                    }
                }

                {
                    SDL_LockMutex(workerPtr->gameMtx);
                    workerPtr->pendingWorkCount -= 1;

                    if (workerPtr->pendingWorkCount <= 0) {

                        SDL_SignalCondition(workerPtr->gameCond);
                    }
#if defined(MAZE_DEBUG)

                    SDL_Log("Pending work count: %d\n", workerPtr->pendingWorkCount);
#endif

                    SDL_UnlockMutex(workerPtr->gameMtx);
                }
            }
    
            return 0;
        }

        return -1;
        }; // lambda


    static constexpr auto NUM_WORKERS = 4;

    // Create worker threads
    for (auto w{ 0 }; w < NUM_WORKERS; w++) {

        string name = { "thread: " + to_string(w) };
        SDL_Thread* t = SDL_CreateThread(threadFunc, name.data(), this);

        if (!t) {
            // Handle thread creation failure
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateThread failed: %s\n", SDL_GetError());
        }

        threads.push_back(t);
    }
}

void WorkerConcurrent::generate(const float maxTime) noexcept {
    using std::size_t;

    if (this->pendingWorkCount == 0) {

        SDL_WaitCondition(gameCond, gameMtx);
    }

    if (maxTime <= 0.f) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Max time is non-positive, cannot track\n");

        return;
    }

    workQueue.clear();

    static constexpr auto BLOCK_COUNT = 4;

    size_t charsPerWorker = static_cast<size_t>(maxTime / static_cast<float>(BLOCK_COUNT));

    for (auto w = 0; w < BLOCK_COUNT; w++) {

        float startTime = w * charsPerWorker;
        float endTime = (w == BLOCK_COUNT - 1) ? maxTime : (w + 1) * charsPerWorker;

        size_t startIdx = static_cast<size_t>(startTime);
        size_t endIdx = static_cast<size_t>(endTime);
        size_t count = endIdx - startIdx;

        SDL_Log("Worker %d: Processing from %zu to %zu (count: %zu)\n", w, startIdx, endIdx, count);

        workQueue.push_back({
            maxTime,
            0.0f,
            static_cast<int>(startIdx),
            static_cast<int>(count)
            });
    }

    this->pendingWorkCount = BLOCK_COUNT;

    SDL_LockMutex(this->gameMtx);
    SDL_SignalCondition(gameCond);
    SDL_UnlockMutex(this->gameMtx);
} // generate

void WorkerConcurrent::doWork(const float& elapsedTime, WorkItem const& item) const noexcept {

    // Simulate work being done
    SDL_Delay(static_cast<std::uint32_t>(elapsedTime * 1000));

    SDL_Log("Processing work item from %d to %d\n", item.start, item.start + item.count);
}

bool WorkerConcurrent::isDone() const noexcept {

    SDL_LockMutex(this->gameMtx);
    bool done = (this->pendingWorkCount <= 0);
    SDL_UnlockMutex(this->gameMtx);

    return done;
}

float WorkerConcurrent::getCompletion() const noexcept {

    SDL_LockMutex(this->gameMtx);
    float completion = 1.0f - (static_cast<float>(this->pendingWorkCount) / static_cast<float>(this->workQueue.size() + this->pendingWorkCount));
    SDL_UnlockMutex(this->gameMtx);

    return completion;
}