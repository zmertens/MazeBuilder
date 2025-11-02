#include "WorkerConcurrent.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iterator>
#include <string>
#include <unordered_map>

#include <SDL3/SDL.h>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create.h>
#include <MazeBuilder/create2.h>
#include <MazeBuilder/string_utils.h>

#include "JsonUtils.hpp"

WorkerConcurrent::WorkItem::WorkItem(std::string key, std::string value, int index)
    : key(std::move(key)), value(std::move(value)), index(index) {

}

// Constructor
WorkerConcurrent::WorkerConcurrent()
    : gameMtx(SDL_CreateMutex())
    , gameCond(SDL_CreateCondition())
    , pendingWorkCount(0)
    , shouldExit(false)
    , mResources{}
    , mTotalWorkItems(0) {

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

                        auto tempWorker = workerPtr->workQueue.front();
                        workerPtr->workQueue.pop_front();

                        SDL_UnlockMutex(workerPtr->gameMtx);

                        // Process the work item
                        workerPtr->doWork(tempWorker);

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

void WorkerConcurrent::generate(std::string_view resourcePath) noexcept {
    using std::string;
    using std::unordered_map;

    if (resourcePath.empty()) {
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
    
    try {
        jsonUtils.loadConfiguration(string(resourcePath), resources);
        SDL_Log("Loaded %zu resources from %s\n", resources.size(), resourcePath.data());
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load resources: %s\n", e.what());
        return;
    }

    if (resources.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No resources found in %s\n", resourcePath.data());
        return;
    }

    // Create work items - distribute resources among workers
    SDL_LockMutex(gameMtx);
    
    int index = 0;
    for (const auto& [key, value] : resources) {
        workQueue.push_back(WorkItem{key, value, index++});
    }
    
    mTotalWorkItems = static_cast<int>(workQueue.size());
    pendingWorkCount = mTotalWorkItems;
    
    SDL_Log("Created %d work items for resource loading\n", mTotalWorkItems);
    
    SDL_BroadcastCondition(gameCond);
    SDL_UnlockMutex(gameMtx);
}

void WorkerConcurrent::doWork(WorkItem const& item) noexcept {

    // Simulate resource loading work (checking, validating, etc.)
    SDL_Delay(100); // Small delay to simulate I/O

    SDL_Log("Processing resource [%d]: %s = %s\n", item.index, item.key.c_str(), item.value.c_str());
    
    // Store the processed resource in a thread-safe manner
    SDL_LockMutex(gameMtx);
    mResources[item.key] = item.value;
    
    // Check if this is a config* key (config1, config2, etc.) and hasn't been processed yet
    // Must start with "config" and be followed by a digit
    bool isConfigKey = false;
    if (item.key.size() >= 7 && item.key.substr(0, 6) == "config") {
        // Check if character after "config" is a digit
        char nextChar = item.key[6];
        isConfigKey = (nextChar >= '0' && nextChar <= '9');
    }
    bool alreadyProcessed = mProcessedConfigs.find(item.key) != mProcessedConfigs.end();
    
    if (isConfigKey && !alreadyProcessed) {
        // Mark as processed immediately to prevent duplicate processing
        mProcessedConfigs[item.key] = true;
        
        // Release mutex before doing expensive work
        SDL_UnlockMutex(gameMtx);
        
        // Helper lambda to convert JSON object string to configurator
        auto jsonToConfigurator = [](const std::string& jsonValue) -> mazes::configurator {
            mazes::configurator config;
            
            // Parse the JSON object string to extract configuration values
            // Expected format: {"rows": 100, "columns": 99, "seed": 50, "algo": "dfs"}
            
            // Extract rows
            if (auto rowsPos = jsonValue.find("\"rows\""); rowsPos != std::string::npos) {
                auto colonPos = jsonValue.find(':', rowsPos);
                if (colonPos != std::string::npos) {
                    auto commaPos = jsonValue.find(',', colonPos);
                    auto value = jsonValue.substr(colonPos + 1, commaPos - colonPos - 1);
                    // Remove whitespace
                    value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
                    try {
                        config.rows(static_cast<unsigned int>(std::stoi(value)));
                    } catch (...) {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to parse rows from: %s\n", value.c_str());
                    }
                }
            }
            
            // Extract columns
            if (auto colsPos = jsonValue.find("\"columns\""); colsPos != std::string::npos) {
                auto colonPos = jsonValue.find(':', colsPos);
                if (colonPos != std::string::npos) {
                    auto commaPos = jsonValue.find(',', colonPos);
                    auto value = jsonValue.substr(colonPos + 1, commaPos - colonPos - 1);
                    value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
                    try {
                        config.columns(static_cast<unsigned int>(std::stoi(value)));
                    } catch (...) {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to parse columns from: %s\n", value.c_str());
                    }
                }
            }
            
            // Extract seed
            if (auto seedPos = jsonValue.find("\"seed\""); seedPos != std::string::npos) {
                auto colonPos = jsonValue.find(':', seedPos);
                if (colonPos != std::string::npos) {
                    auto commaPos = jsonValue.find(',', colonPos);
                    if (commaPos == std::string::npos) {
                        commaPos = jsonValue.find('}', colonPos);
                    }
                    auto value = jsonValue.substr(colonPos + 1, commaPos - colonPos - 1);
                    value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
                    try {
                        config.seed(static_cast<unsigned int>(std::stoi(value)));
                    } catch (...) {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to parse seed from: %s\n", value.c_str());
                    }
                }
            }
            
            // Extract algo
            if (auto algoPos = jsonValue.find("\"algo\""); algoPos != std::string::npos) {
                auto colonPos = jsonValue.find(':', algoPos);
                if (colonPos != std::string::npos) {
                    auto quoteStart = jsonValue.find('"', colonPos);
                    if (quoteStart != std::string::npos) {
                        auto quoteEnd = jsonValue.find('"', quoteStart + 1);
                        if (quoteEnd != std::string::npos) {
                            auto algoStr = jsonValue.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                            
                            // Map string to algo enum
                            if (algoStr == "dfs") {
                                config.algo_id(mazes::algo::DFS);
                            } else if (algoStr == "binary_tree") {
                                config.algo_id(mazes::algo::BINARY_TREE);
                            } else if (algoStr == "sidewinder") {
                                config.algo_id(mazes::algo::SIDEWINDER);
                            } else {
                                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unknown algorithm: %s, using default\n", algoStr.c_str());
                            }
                        }
                    }
                }
            }
            
            return config;
        };
        
        // Convert the JSON value to a configurator
        mazes::configurator config = jsonToConfigurator(item.value);
        
        // Call create() and log the result
        try {
            std::string mazeStr = mazes::create(config);
            SDL_Log("Generated maze for %s:\n%s\n", item.key.c_str(), mazeStr.c_str());
        } catch (const std::exception& e) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create maze for %s: %s\n", item.key.c_str(), e.what());
        }
    } else {
        SDL_UnlockMutex(gameMtx);
    }
}

bool WorkerConcurrent::isDone() const noexcept {

    SDL_LockMutex(this->gameMtx);
    bool done = (this->pendingWorkCount <= 0);
    SDL_UnlockMutex(this->gameMtx);

    return done;
}

float WorkerConcurrent::getCompletion() const noexcept {

    SDL_LockMutex(this->gameMtx);
    float completion = 0.0f;
    if (mTotalWorkItems > 0) {
        int completed = mTotalWorkItems - pendingWorkCount;
        completion = static_cast<float>(completed) / static_cast<float>(mTotalWorkItems);
    }
    SDL_UnlockMutex(this->gameMtx);

    return completion;
}

std::unordered_map<std::string, std::string> WorkerConcurrent::getResources() const noexcept {
    
    SDL_LockMutex(gameMtx);
    auto resourcesCopy = mResources;
    SDL_UnlockMutex(gameMtx);
    
    return resourcesCopy;
}