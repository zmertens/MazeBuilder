#ifndef WORKER_CONCURRENT
#define WORKER_CONCURRENT

#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL_atomic.h>

#include "ResourceIdentifiers.hpp"

struct SDL_Thread;
struct SDL_Mutex;
struct SDL_Condition;
struct SDL_Vertex;

namespace Textures
{
    enum class ID : unsigned int;
}

/// @brief Provides concurrent worker threads for string processing
/// @details This class manages a queue of work items and spawns multiple threads to process them concurrently.
/// @details Each thread processes a segment of a string, setting vertices for rendering.
class WorkerConcurrent
{
public:
    /// @brief Represents a texture that needs to be loaded on the main thread
    struct TextureLoadRequest
    {
        Textures::ID id;
        std::string path;

        TextureLoadRequest(Textures::ID id, std::string path);
    };

    explicit WorkerConcurrent();
    ~WorkerConcurrent();
    WorkerConcurrent(const WorkerConcurrent& other) = delete;
    WorkerConcurrent& operator=(const WorkerConcurrent& other) = delete;
    WorkerConcurrent(WorkerConcurrent&& other) noexcept = default;
    WorkerConcurrent& operator=(WorkerConcurrent&& other) = delete;

    void initThreads() noexcept;
    void generate(std::string_view resourcePath) noexcept;
    bool isDone() const noexcept;
    float getCompletion() const noexcept;

    // Get the loaded resources (thread-safe)
    std::unordered_map<std::string, std::string> getResources() const noexcept;

    // Get texture load requests collected by worker threads (thread-safe)
    std::vector<TextureLoadRequest> getTextureLoadRequests() const noexcept;

    // Set the resource path prefix for resolving relative paths
    void setResourcePathPrefix(std::string_view prefix) noexcept;

    // Get composed maze strings (thread-safe)
    std::unordered_map<Textures::ID, std::string> getComposedMazeStrings() const noexcept;

private:
    struct WorkItem
    {
        struct JSONKeyMapping
        {
            std::string_view key;
            Textures::ID id;
        };

        std::string key;
        std::string value;
        int index;

        WorkItem(std::string key, std::string value, int index);
    };

    void doWork(WorkItem const& workItem) noexcept;

    const std::vector<WorkItem::JSONKeyMapping> mConfigMappings;

    std::deque<WorkItem> mWorkQueue;
    std::vector<SDL_Thread*> mThreads;
    SDL_Mutex* mGameMtx;
    SDL_Condition* mGameCond;
    int mPendingWorkCount;
    SDL_AtomicInt mShouldExit;

    // Store loaded resources
    std::unordered_map<std::string, std::string> mResources;
    int mTotalWorkItems;

    // Track which config* keys have been processed to ensure one-time execution
    std::unordered_map<std::string, bool> mProcessedConfigs;

    // Texture load requests collected by worker threads
    std::vector<TextureLoadRequest> mTextureLoadRequests;

    // Resource path prefix for resolving relative paths
    std::string mResourcePathPrefix;

    // Composed maze strings for level textures (ID -> maze string)
    std::unordered_map<Textures::ID, std::string> mComposedMazeStrings;
};

#endif // WORKER_CONCURRENT
