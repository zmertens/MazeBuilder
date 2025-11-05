// Provides thread-based concurrency for managing resource loading and configuration.

#include "WorkerConcurrent.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_map>

#include <SDL3/SDL.h>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create.h>
#include <MazeBuilder/create2.h>

#include "JsonUtils.hpp"

WorkerConcurrent::WorkItem::WorkItem(std::string key, std::string value, int index)
    : key(std::move(key)), value(std::move(value)), index(index)
{
}

// Constructor
WorkerConcurrent::WorkerConcurrent()
    : gameMtx(SDL_CreateMutex())
      , gameCond(SDL_CreateCondition())
      , pendingWorkCount(0)
      , shouldExit{}
      , mResources{}
      , mTotalWorkItems(0)
{
    // Initialize atomic int to 0 (false)
    SDL_SetAtomicInt(&shouldExit, 0);

    if (!gameCond)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating condition variable: %s\n", SDL_GetError());
    }

    if (!gameMtx)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating mutex: %s\n", SDL_GetError());
    }
}

// Destructor
WorkerConcurrent::~WorkerConcurrent()
{
    // Signal threads to exit using atomic operation
    SDL_SetAtomicInt(&shouldExit, 1);

    SDL_LockMutex(gameMtx);
    // Clear work queue to prevent any new work from being picked up
    workQueue.clear();
    pendingWorkCount = 0;
    SDL_BroadcastCondition(gameCond);
    SDL_UnlockMutex(gameMtx);

    // Give threads a moment to see the exit signal
    SDL_Delay(50);

    // Wait for all threads to finish
    for (auto t : threads)
    {
        if (t)
        {
            auto name = SDL_GetThreadName(t);
            int status = 0;
            SDL_WaitThread(t, &status);
            SDL_Log("Worker thread with status [ %s | %d ] finished\n", name, status);
        }
    }

    // Clear threads vector
    threads.clear();

    // Now it's safe to destroy synchronization primitives
    if (gameMtx)
    {
        SDL_DestroyMutex(gameMtx);
        gameMtx = nullptr;
    }

    if (gameCond)
    {
        SDL_DestroyCondition(gameCond);
        gameCond = nullptr;
    }
}

// Copy Constructor
WorkerConcurrent::WorkerConcurrent(const WorkerConcurrent& other)
    : workQueue(other.workQueue), pendingWorkCount(other.pendingWorkCount)
{
    gameMtx = SDL_CreateMutex();
    gameCond = SDL_CreateCondition();

    if (!gameMtx || !gameCond)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating SDL concurrency objects: %s\n", SDL_GetError());
    }
    // Threads are not copied as they cannot be shared between instances
}

// Copy Assignment Operator
WorkerConcurrent& WorkerConcurrent::operator=(const WorkerConcurrent& other)
{
    if (this == &other)
    {
        return *this;
    }

    // Clean up existing resources
    for (auto thread : threads)
    {
        if (thread)
        {
            SDL_WaitThread(thread, nullptr);
        }
    }
    SDL_DestroyMutex(gameMtx);
    SDL_DestroyCondition(gameCond);

    // Copy data
    for (auto&& item : other.workQueue)
    {
        workQueue.push_back(item);
    }
    pendingWorkCount = other.pendingWorkCount;

    // Reinitialize mutex and condition variable
    gameMtx = SDL_CreateMutex();
    gameCond = SDL_CreateCondition();

    if (!gameMtx || !gameCond)
    {
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
      , pendingWorkCount(other.pendingWorkCount)
{
    other.gameMtx = nullptr;
    other.gameCond = nullptr;
}

// Move Assignment Operator
WorkerConcurrent& WorkerConcurrent::operator=(WorkerConcurrent&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    // Clean up existing resources
    for (auto thread : threads)
    {
        if (thread)
        {
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
void WorkerConcurrent::initThreads() noexcept
{
    using std::back_inserter;
    using std::copy;
    using std::cref;
    using std::ref;
    using std::string;
    using std::to_string;
    using std::vector;

    auto threadFunc = [](void* data) -> int
    {
        if (auto workerPtr = reinterpret_cast<WorkerConcurrent*>(data))
        {
            while (SDL_GetAtomicInt(&workerPtr->shouldExit) == 0)
            {
                SDL_LockMutex(workerPtr->gameMtx);

                while (workerPtr->workQueue.empty() && SDL_GetAtomicInt(&workerPtr->shouldExit) == 0)
                {
                    SDL_WaitCondition(workerPtr->gameCond, workerPtr->gameMtx);
                }

                // Check if we should exit before processing
                if (SDL_GetAtomicInt(&workerPtr->shouldExit) != 0)
                {
                    SDL_UnlockMutex(workerPtr->gameMtx);
                    break;
                }

                std::optional<WorkItem> tempWorker;
                bool hasWork = false;

                if (!workerPtr->workQueue.empty())
                {
                    tempWorker = workerPtr->workQueue.front();
                    workerPtr->workQueue.pop_front();
                    hasWork = true;
                }

                SDL_UnlockMutex(workerPtr->gameMtx);

                // Process work outside the lock
                if (hasWork && SDL_GetAtomicInt(&workerPtr->shouldExit) == 0 && tempWorker.has_value())
                {
                    workerPtr->doWork(tempWorker.value());
                }

                // Update pending work count
                SDL_LockMutex(workerPtr->gameMtx);
                if (hasWork)
                {
                    workerPtr->pendingWorkCount -= 1;

                    if (workerPtr->pendingWorkCount <= 0)
                    {
                        SDL_SignalCondition(workerPtr->gameCond);
                    }
                }
                SDL_UnlockMutex(workerPtr->gameMtx);
            }

            return 0;
        }

        // Cast failed
        return -1;
    }; // lambda


    static constexpr auto NUM_WORKERS = 4;

    // Create worker threads
    for (auto w{0}; w < NUM_WORKERS; w++)
    {
        string name = {"thread: " + to_string(w)};
        SDL_Thread* t = SDL_CreateThread(threadFunc, name.data(), this);

        if (!t)
        {
            // Handle thread creation failure
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateThread failed: %s\n", SDL_GetError());
        }

        threads.push_back(t);
    }
}

void WorkerConcurrent::generate(std::string_view resourcePath) noexcept
{
    using std::string;
    using std::unordered_map;

    if (resourcePath.empty())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Resource path is empty\n");
        return;
    }

    SDL_LockMutex(gameMtx);
    workQueue.clear();
    mResources.clear();
    mProcessedConfigs.clear(); // Clear processed configs tracking
    SDL_UnlockMutex(gameMtx);

    // Load JSON configuration
    JsonUtils jsonUtils{};
    unordered_map<string, string> resources{};

    try
    {
        jsonUtils.loadConfiguration(string(resourcePath), resources);
        SDL_Log("Loaded %zu resources from %s\n", resources.size(), resourcePath.data());
    }
    catch (const std::exception& e)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load resources: %s\n", e.what());
        return;
    }

    if (resources.empty())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No resources found in %s\n", resourcePath.data());
        return;
    }

    // Create work items - distribute resources among workers
    SDL_LockMutex(gameMtx);

    int index = 0;
    for (const auto& [key, value] : resources)
    {
        workQueue.emplace_back(key, value, index++);
    }

    mTotalWorkItems = static_cast<int>(workQueue.size());
    pendingWorkCount = mTotalWorkItems;

    SDL_Log("Created %d work items for resource loading\n", mTotalWorkItems);

    SDL_BroadcastCondition(gameCond);
    SDL_UnlockMutex(gameMtx);
}

void WorkerConcurrent::doWork(WorkItem const& item) noexcept
{
    // Early exit check before any work
    if (SDL_GetAtomicInt(&shouldExit) != 0)
    {
        return;
    }

#if defined(MAZE_DEBUG)

    SDL_Log("Processing resource [%d]: %s = %s\n", item.index, item.key.c_str(), item.value.c_str());
#endif

    // Check if this is a config* key before acquiring expensive locks
    bool isConfigKey = false;
    if (item.key.size() >= 7 && item.key.substr(0, 6) == "config")
    {
        char nextChar = item.key[6];
        isConfigKey = (nextChar >= '0' && nextChar <= '9');
    }

    // Store the processed resource in a thread-safe manner
    SDL_LockMutex(gameMtx);

    // Double-check if we're shutting down
    if (SDL_GetAtomicInt(&shouldExit) != 0)
    {
        SDL_UnlockMutex(gameMtx);
        return;
    }

    mResources[item.key] = item.value;

    bool alreadyProcessed = mProcessedConfigs.find(item.key) != mProcessedConfigs.end();

    if (isConfigKey && !alreadyProcessed)
    {
        // Mark as processed immediately to prevent duplicate processing
        mProcessedConfigs[item.key] = true;

        // Get the JSON value while we still hold the lock
        std::string jsonValue = item.value;

        // Release mutex before doing expensive work
        SDL_UnlockMutex(gameMtx);

        // Check again if we're shutting down before expensive operation
        if (SDL_GetAtomicInt(&shouldExit) != 0)
        {
            return;
        }

        // Call create() and log the result
        try
        {
            [[maybe_unused]]
                std::string mazeStr = mazes::create(JsonUtils::jsonToConfigurator(jsonValue));
        }
        catch (const std::exception& e)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create maze for %s: %s\n", item.key.c_str(), e.what());
        }
    }
    else
    {
        SDL_UnlockMutex(gameMtx);
    }
}

bool WorkerConcurrent::isDone() const noexcept
{
    SDL_LockMutex(this->gameMtx);
    bool done = (this->pendingWorkCount <= 0);
    SDL_UnlockMutex(this->gameMtx);

    return done;
}

float WorkerConcurrent::getCompletion() const noexcept
{
    SDL_LockMutex(this->gameMtx);
    float completion = 0.0f;
    if (mTotalWorkItems > 0)
    {
        int completed = mTotalWorkItems - pendingWorkCount;
        completion = static_cast<float>(completed) / static_cast<float>(mTotalWorkItems);
    }
    SDL_UnlockMutex(this->gameMtx);

    return completion;
}

std::unordered_map<std::string, std::string> WorkerConcurrent::getResources() const noexcept
{
    SDL_LockMutex(gameMtx);
    auto resourcesCopy = mResources;
    SDL_UnlockMutex(gameMtx);

    return resourcesCopy;
}
