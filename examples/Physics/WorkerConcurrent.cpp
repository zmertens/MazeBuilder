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
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder/json_helper.h>

#include "JsonUtils.hpp"
#include "ResourceIdentifiers.hpp"

WorkerConcurrent::WorkItem::WorkItem(std::string key, std::string value, int index)
    : key(std::move(key)), value(std::move(value)), index(index)
{
}

WorkerConcurrent::TextureLoadRequest::TextureLoadRequest(Textures::ID id, std::string path)
    : id(id), path(std::move(path))
{
}

// Constructor
WorkerConcurrent::WorkerConcurrent()
    : mConfigMappings({
          {JSONKeys::BALL_NORMAL, Textures::ID::BALL_NORMAL},
          {JSONKeys::CHARACTER_IMAGE, Textures::ID::CHARACTER},
          {JSONKeys::LEVEL_DEFAULTS, Textures::ID::LEVEL_TWO},
          {JSONKeys::CHARACTERS_SPRITE_SHEET, Textures::ID::CHARACTER_SPRITE_SHEET},
          {JSONKeys::CHARACTER_IMAGE, Textures::ID::CHARACTER},
          {JSONKeys::CHARACTER_IMAGE, Textures::ID::CHARACTER},
          {JSONKeys::SPLASH_IMAGE, Textures::ID::SPLASH_TITLE_IMAGE},
          {JSONKeys::SDL_LOGO, Textures::ID::SDL_LOGO},
          {JSONKeys::SFML_LOGO, Textures::ID::SFML_LOGO},
          {JSONKeys::WALL_HORIZONTAL, Textures::ID::WALL_HORIZONTAL},
          {JSONKeys::WINDOW_ICON, Textures::ID::WINDOW_ICON}
      }), mGameMtx(SDL_CreateMutex())
      , mGameCond(SDL_CreateCondition())
      , mPendingWorkCount(0)
      , mShouldExit{}
      , mResources{}
      , mTotalWorkItems(0)
{
    // Initialize atomic int to 0 (false)
    SDL_SetAtomicInt(&mShouldExit, 0);

    if (!mGameCond)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating condition variable: %s\n", SDL_GetError());
    }

    if (!mGameMtx)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error creating mutex: %s\n", SDL_GetError());
    }
}

// Destructor
WorkerConcurrent::~WorkerConcurrent()
{
    // Signal mThreads to exit using atomic operation
    SDL_SetAtomicInt(&mShouldExit, 1);

    SDL_LockMutex(mGameMtx);
    // Clear work queue to prevent any new work from being picked up
    mWorkQueue.clear();
    mPendingWorkCount = 0;
    SDL_BroadcastCondition(mGameCond);
    SDL_UnlockMutex(mGameMtx);

    // Give mThreads a moment to see the exit signal
    SDL_Delay(50);

    // Wait for all mThreads to finish
    for (const auto t : mThreads)
    {
        if (t)
        {
            const auto name = SDL_GetThreadName(t);
            int status = 0;
            SDL_WaitThread(t, &status);
            SDL_Log("Worker thread with status [ %s | %d ] finished\n", name, status);
        }
    }

    // Clear mThreads vector
    mThreads.clear();

    // Now it's safe to destroy synchronization primitives
    if (mGameMtx)
    {
        SDL_DestroyMutex(mGameMtx);
        mGameMtx = nullptr;
    }

    if (mGameCond)
    {
        SDL_DestroyCondition(mGameCond);
        mGameCond = nullptr;
    }
}

// Initialize worker mThreads
void WorkerConcurrent::initThreads() noexcept
{
    using std::back_inserter;
    using std::copy;
    using std::cref;
    using std::optional;
    using std::ref;
    using std::string;
    using std::to_string;
    using std::vector;

    auto threadFunc = [](void* data) -> int
    {
        if (const auto workerPtr = static_cast<WorkerConcurrent*>(data))
        {
            while (SDL_GetAtomicInt(&workerPtr->mShouldExit) == 0)
            {
                SDL_LockMutex(workerPtr->mGameMtx);

                while (workerPtr->mWorkQueue.empty() && SDL_GetAtomicInt(&workerPtr->mShouldExit) == 0)
                {
                    SDL_WaitCondition(workerPtr->mGameCond, workerPtr->mGameMtx);
                }

                // Check if we should exit before processing
                if (SDL_GetAtomicInt(&workerPtr->mShouldExit) != 0)
                {
                    SDL_UnlockMutex(workerPtr->mGameMtx);
                    break;
                }

                optional<WorkItem> tempWorker;
                bool hasWork = false;

                if (!workerPtr->mWorkQueue.empty())
                {
                    tempWorker = workerPtr->mWorkQueue.front();
                    workerPtr->mWorkQueue.pop_front();
                    hasWork = true;
                }

                SDL_UnlockMutex(workerPtr->mGameMtx);

                // Process work outside the lock
                if (hasWork && SDL_GetAtomicInt(&workerPtr->mShouldExit) == 0 && tempWorker.has_value())
                {
                    workerPtr->doWork(cref(tempWorker.value()));
                }

                // Update pending work count
                SDL_LockMutex(workerPtr->mGameMtx);
                if (hasWork)
                {
                    workerPtr->mPendingWorkCount -= 1;

                    if (workerPtr->mPendingWorkCount <= 0)
                    {
                        SDL_SignalCondition(workerPtr->mGameCond);
                    }
                }
                SDL_UnlockMutex(workerPtr->mGameMtx);
            }

            return 0;
        }

        // Cast failed
        return -1;
    }; // lambda

    static constexpr auto NUM_WORKERS = 4;

    // Create worker mThreads
    for (auto w{0}; w < NUM_WORKERS; w++)
    {
        string name = {"thread: " + to_string(w)};
        SDL_Thread* t = SDL_CreateThread(threadFunc, name.data(), this);

        if (!t)
        {
            // Handle thread creation failure
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateThread failed: %s\n", SDL_GetError());
        }

        mThreads.push_back(t);
    }
}

void WorkerConcurrent::generate(std::string_view resourcePath) noexcept
{
    using std::exception;
    using std::ref;
    using std::string;
    using std::unordered_map;

    if (resourcePath.empty())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Resource path is empty\n");
        return;
    }

    SDL_LockMutex(mGameMtx);
    mWorkQueue.clear();
    mResources.clear();
    mProcessedConfigs.clear();
    mTextureLoadRequests.clear();
    mComposedMazeStrings.clear();
    SDL_UnlockMutex(mGameMtx);

    // Store the resource path prefix for use in worker mThreads
    mResourcePathPrefix = mazes::io_utils::getDirectoryPath(string{resourcePath}) + "/";

    // Load JSON configuration
    unordered_map<string, string> resources{};

    try
    {
        JSONUtils::loadConfiguration(string{resourcePath}, ref(resources));

#if defined(MAZE_DEBUG)

        SDL_Log("Loaded %zu resources from %s\n", resources.size(), resourcePath.data());
#endif
    }
    catch (const exception& e)
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
    SDL_LockMutex(mGameMtx);

    int index = 0;
    for (const auto& [key, value] : resources)
    {
        mWorkQueue.emplace_back(key, value, index++);
    }

    mTotalWorkItems = static_cast<int>(mWorkQueue.size());
    mPendingWorkCount = mTotalWorkItems;

#if defined(MAZE_DEBUG)

    SDL_Log("Created %d work items for resource loading\n", mTotalWorkItems);
#endif

    SDL_BroadcastCondition(mGameCond);
    SDL_UnlockMutex(mGameMtx);
}

void WorkerConcurrent::doWork(WorkItem const& workItem) noexcept
{
    // Early exit check before any work
    if (SDL_GetAtomicInt(&mShouldExit) != 0)
    {
        return;
    }

#if defined(MAZE_DEBUG)

    SDL_Log("Processing resource [%d]: %s = %s\n", workItem.index, workItem.key.c_str(), workItem.value.c_str());
#endif

    // Store the processed resource in a thread-safe manner
    SDL_LockMutex(mGameMtx);

    // Double-check if we're shutting down
    if (SDL_GetAtomicInt(&mShouldExit) != 0)
    {
        SDL_UnlockMutex(mGameMtx);
        return;
    }

    mResources[workItem.key] = workItem.value;

    // Special handling for level_defaults - it's an array of maze configurations
    if (workItem.key == JSONKeys::LEVEL_DEFAULTS)
    {
        SDL_UnlockMutex(mGameMtx);

        // Process level configurations outside the lock (expensive operation)
        std::vector<std::unordered_map<std::string, std::string>> levelConfigs;
        mazes::json_helper jh{};

        if (jh.from_array(workItem.value, levelConfigs))
        {
            std::string composedMazeString;

#if defined(MAZE_DEBUG)
            SDL_Log("Processing %zu level configurations from level_defaults\n", levelConfigs.size());
#endif

            // Create maze for each configuration
            for (size_t i = 0; i < levelConfigs.size(); ++i)
            {
                try
                {
                    // Convert the config map to a configurator
                    mazes::configurator config;

                    for (const auto& [key, value] : levelConfigs[i])
                    {
                        // Parse integer fields
                        if (key == "rows" || key == "columns" || key == "seed")
                        {
                            try
                            {
                                int intValue = std::stoi(JSONUtils::extractJsonValue(value));
                                if (key == "rows")
                                    config.rows(static_cast<unsigned int>(intValue));
                                else if (key == "columns")
                                    config.columns(static_cast<unsigned int>(intValue));
                                else if (key == "seed")
                                    config.seed(static_cast<unsigned int>(intValue));
                            }
                            catch (...)
                            {
                                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to parse %s from level config %zu\n", key.c_str(), i);
                            }
                        }
                        // Parse algo string
                        else if (key == "algo")
                        {
                            std::string algoStr = JSONUtils::extractJsonValue(value);
                            config.algo_id(mazes::to_algo_from_sv(algoStr));
                        }
                        // Parse distances boolean
                        else if (key == "distances")
                        {
                            // nlohmann::json dumps booleans as "true" or "false" (no quotes)
                            // But extractJsonValue might add/remove quotes, so check both
                            std::string distValue = value;
                            // Remove any quotes or whitespace
                            distValue.erase(std::remove_if(distValue.begin(), distValue.end(),
                                [](char c) { return c == '"' || c == '\'' || std::isspace(static_cast<unsigned char>(c)); }),
                                distValue.end());

                            bool showDistances = (distValue == "true" || distValue == "1");
                            if (showDistances)
                            {
                                config.distances("[0:-1]");
                            }
                        }
                    }

                    // Create maze from configuration
                    std::string mazeStr = mazes::create(config);

                    if (!mazeStr.empty())
                    {
                        // Add separator between mazes if not the first one
                        if (!composedMazeString.empty())
                        {
                            composedMazeString += "\n\n";
                        }
                        composedMazeString += mazeStr;

#if defined(MAZE_DEBUG)
                        SDL_Log("Generated maze %zu: %zu characters\n", i, mazeStr.size());
#endif
                    }
                }
                catch (const std::exception& e)
                {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create maze from config %zu: %s\n", i, e.what());
                }
            }

            // Store the composed maze string
            if (!composedMazeString.empty())
            {
                SDL_LockMutex(mGameMtx);
                // Use LEVEL_TWO as the ID for the composed maze (you can change this mapping)
                mComposedMazeStrings[Textures::ID::LEVEL_TWO] = composedMazeString;

#if defined(MAZE_DEBUG)
                SDL_Log("Composed maze string: %zu total characters\n", composedMazeString.size());
#endif

                SDL_UnlockMutex(mGameMtx);
            }
        }
        else
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to parse level_defaults array\n");
        }

        SDL_LockMutex(mGameMtx);
    }
    else if (workItem.key == JSONKeys::PLAYER_HITPOINTS_DEFAULT)
    {

    }
    else if (workItem.key == JSONKeys::PLAYER_SPEED_DEFAULT)
    {

    }
    else if (workItem.key == JSONKeys::ENEMY_HITPOINTS_DEFAULT)
    {

    }
    else if (workItem.key == JSONKeys::ENEMY_SPEED_DEFAULT)
    {

    }
    else
    {
        // Regular texture path handling
        for (const auto& [key, id] : mConfigMappings)
        {
            if (workItem.key == key)
            {
                mTextureLoadRequests.emplace_back(id, mResourcePathPrefix + JSONUtils::extractJsonValue(workItem.value));
                break;
            }
        }
    }

    if (const bool alreadyProcessed = mProcessedConfigs.contains(workItem.key); !alreadyProcessed)
    {
        // Mark as processed immediately to prevent duplicate processing
        mProcessedConfigs[workItem.key] = true;
    }

    SDL_UnlockMutex(mGameMtx);
}

bool WorkerConcurrent::isDone() const noexcept
{
    SDL_LockMutex(this->mGameMtx);
    const bool done = (this->mPendingWorkCount <= 0);
    SDL_UnlockMutex(this->mGameMtx);

    return done;
}

float WorkerConcurrent::getCompletion() const noexcept
{
    SDL_LockMutex(this->mGameMtx);
    float completion = 0.0f;
    if (mTotalWorkItems > 0)
    {
        const int completed = mTotalWorkItems - mPendingWorkCount;
        completion = static_cast<float>(completed) / static_cast<float>(mTotalWorkItems);
    }
    SDL_UnlockMutex(this->mGameMtx);

    return completion;
}

std::unordered_map<std::string, std::string> WorkerConcurrent::getResources() const noexcept
{
    SDL_LockMutex(mGameMtx);
    auto resourcesCopy = mResources;
    SDL_UnlockMutex(mGameMtx);

    return resourcesCopy;
}

std::vector<WorkerConcurrent::TextureLoadRequest> WorkerConcurrent::getTextureLoadRequests() const noexcept
{
    SDL_LockMutex(mGameMtx);
    auto requestsCopy = mTextureLoadRequests;
    SDL_UnlockMutex(mGameMtx);

    return requestsCopy;
}

void WorkerConcurrent::setResourcePathPrefix(std::string_view prefix) noexcept
{
    SDL_LockMutex(mGameMtx);
    mResourcePathPrefix = prefix;
    SDL_UnlockMutex(mGameMtx);
}

std::unordered_map<Textures::ID, std::string> WorkerConcurrent::getComposedMazeStrings() const noexcept
{
    SDL_LockMutex(mGameMtx);
    auto mazesCopy = mComposedMazeStrings;
    SDL_UnlockMutex(mGameMtx);

    return mazesCopy;
}

