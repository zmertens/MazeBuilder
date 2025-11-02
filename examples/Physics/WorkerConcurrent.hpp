#ifndef WORKER_CONCURRENT
#define WORKER_CONCURRENT

#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL_atomic.h>

struct SDL_Thread;
struct SDL_Mutex;
struct SDL_Condition;
struct SDL_Vertex;

/// @brief Provides concurrent worker threads for string processing
/// @details This class manages a queue of work items and spawns multiple threads to process them concurrently.
/// @details Each thread processes a segment of a string, setting vertices for rendering.
class WorkerConcurrent {
public:
    explicit WorkerConcurrent();
    ~WorkerConcurrent();
    WorkerConcurrent(const WorkerConcurrent& other);
    WorkerConcurrent& operator=(const WorkerConcurrent& other);
    WorkerConcurrent(WorkerConcurrent&& other) noexcept;
    WorkerConcurrent& operator=(WorkerConcurrent&& other) noexcept;

    void initThreads() noexcept;
    void generate(std::string_view resourcePath) noexcept;
    bool isDone() const noexcept;
    float getCompletion() const noexcept;
    
    // Get the loaded resources (thread-safe)
    std::unordered_map<std::string, std::string> getResources() const noexcept;

private:
    struct WorkItem {
        std::string key;
        std::string value;
        int index;

        WorkItem(std::string key, std::string value, int index);
    };

    void doWork(WorkItem const& workItem) noexcept;

    std::deque<WorkItem> workQueue;
    std::vector<SDL_Thread*> threads;
    SDL_Mutex* gameMtx;
    SDL_Condition* gameCond;
    int pendingWorkCount;
    SDL_AtomicInt shouldExit;
    
    // Store loaded resources
    std::unordered_map<std::string, std::string> mResources;
    int mTotalWorkItems;
    
    // Track which config* keys have been processed to ensure one-time execution
    std::unordered_map<std::string, bool> mProcessedConfigs;
};

#endif // WORKER_CONCURRENT
