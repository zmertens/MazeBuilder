#include "WorkerConcurrent.hpp"

#include <algorithm>
#include <iterator>
#include <string>

#include <SDL3/SDL.h>

// Constructor
WorkerConcurrent::WorkerConcurrent(State& state)
    : state(state)
    , gameMtx(SDL_CreateMutex()), gameCond(SDL_CreateCondition()), pendingWorkCount(0) {

    if (!gameCond) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating condition variable: %s\n", SDL_GetError());
    }

    if (!gameMtx) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating mutex: %s\n", SDL_GetError());
    }
}

// Destructor
WorkerConcurrent::~WorkerConcurrent() {

    state = State::DONE;

    pendingWorkCount = 0;

    SDL_LockMutex(gameMtx);
    SDL_BroadcastCondition(gameCond);
    SDL_UnlockMutex(gameMtx);

    for (auto t : threads) {
        if (t) {

            auto name = SDL_GetThreadName(t);
            int status = 0;
            SDL_WaitThread(t, &status);
            SDL_Log("Worker thread with status [ %s | %d ] to finish\n", name, status);
        }
    }

    SDL_DestroyMutex(gameMtx);
    SDL_DestroyCondition(gameCond);
}

// Copy Constructor
WorkerConcurrent::WorkerConcurrent(const WorkerConcurrent& other)
    : state(other.state), workQueue(other.workQueue), pendingWorkCount(other.pendingWorkCount) {

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
    state = other.state;
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
    : state(other.state)
    , workQueue(std::move(other.workQueue))
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
    state = other.state;
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
    using namespace std;

    auto threadFunc = [](void* data) -> int {

        auto* workerPtr = reinterpret_cast<WorkerConcurrent*>(data);

        vector<SDL_Vertex> vertices;

        while (1) {
            {
                SDL_LockMutex(workerPtr->gameMtx);
                while (workerPtr->workQueue.empty()) {
                    SDL_WaitCondition(workerPtr->gameCond, workerPtr->gameMtx);
                }

                if (workerPtr->state == State::DONE) {
                    SDL_UnlockMutex(workerPtr->gameMtx);
                    break;
                }

                if (!workerPtr->workQueue.empty()) {

                    auto&& tempWorker = workerPtr->workQueue.front();
                    workerPtr->workQueue.pop_front();

                    SDL_Log("Processing work item [ start: %d | count: %d | rows: %d | columns: %d]\n",
                        tempWorker.start, tempWorker.count, tempWorker.rows, tempWorker.columns);

                    vertices.clear();

                    workerPtr->doWork(ref(vertices), cref(tempWorker));

                    SDL_Log("Generated %zu vertices for this work item\n", vertices.size());

                    if (!vertices.empty()) {
                        copy(vertices.begin(), vertices.end(), back_inserter(tempWorker.vertices));
                        SDL_Log("Total vertices after copy: %zu\n", tempWorker.vertices.size());
                    } else {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No vertices generated for this work item\n");
                    }
                }
                SDL_UnlockMutex(workerPtr->gameMtx);
            }

            {
                SDL_LockMutex(workerPtr->gameMtx);
                workerPtr->pendingWorkCount -= 1;
                SDL_Log("Pending work count: %d\n", workerPtr->pendingWorkCount);

                if (workerPtr->pendingWorkCount <= 0) {
                    SDL_SignalCondition(workerPtr->gameCond);
                }

                SDL_UnlockMutex(workerPtr->gameMtx);
            }
        }

        return 0;
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

void WorkerConcurrent::generate(std::string_view tab) noexcept {
    using namespace std;

    if (this->pendingWorkCount == 0) {

        SDL_WaitCondition(gameCond, gameMtx);
    }

    if (tab.empty()) {

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Maze string is empty, cannot generate level\n");

        return;
    }

    workQueue.clear();

    static constexpr auto BLOCK_COUNT = 4;

    size_t charsPerWorker = tab.size() / BLOCK_COUNT;

    vector<SDL_Vertex> vertices;

    for (auto w = 0; w < BLOCK_COUNT; w++) {

        size_t startIdx = w * charsPerWorker;
        size_t endIdx = (w == BLOCK_COUNT - 1) ? tab.size() : (w + 1) * charsPerWorker;
        if (w > 0) {
            while (startIdx > 0 && tab[startIdx] != '\n') {
                startIdx--;
            }
            if (startIdx > 0) startIdx++;
        }
        if (w < BLOCK_COUNT - 1) {
            while (endIdx < tab.size() && tab[endIdx] != '\n') {
                endIdx++;
            }
            if (endIdx < tab.size()) endIdx++;
        }

        size_t count = endIdx - startIdx;

        SDL_Log("Worker %d: Processing from %zu to %zu (count: %zu)\n", w, startIdx, endIdx, count);

        workQueue.push_back({
            cref(tab),
            ref(vertices),
            static_cast<int>(startIdx),
            static_cast<int>(count),
            1,
            1
            });
    }

    this->pendingWorkCount = BLOCK_COUNT;

    SDL_LockMutex(this->gameMtx);
    SDL_SignalCondition(gameCond);
    SDL_UnlockMutex(this->gameMtx);
} // generate

void WorkerConcurrent::doWork(std::vector<SDL_Vertex>& vertices, WorkItem const& item) const noexcept {
    using namespace std;

    SDL_FColor wallColor = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black
    auto pushV = [&vertices](auto v1, auto v2, auto v3, auto v4)->void {
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v4);
        vertices.push_back(v2);
        vertices.push_back(v3);
        vertices.push_back(v4);
        };

    SDL_FColor cellColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    const auto& mazeString = item.sv;

    SDL_Log("Processing maze string segment from %d to %d\n", item.start, item.start + item.count);
}
