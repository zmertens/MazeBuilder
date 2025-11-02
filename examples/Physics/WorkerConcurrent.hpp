#ifndef WORKER_CONCURRENT
#define WORKER_CONCURRENT

#include <deque>
#include <vector>
#include <string_view>

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
    void generate(const float maxTime) noexcept;
    bool isDone() const noexcept;
    float getCompletion() const noexcept;

private:
    struct WorkItem {
        const float maxTime;
        float elapsedTime;
        int start, count;

        WorkItem(const float maxTime, float elapsedTime, int start, int count);
    };

    void doWork(const float& elapsedTime, WorkItem const& workItem) const noexcept;

    std::deque<WorkItem> workQueue;
    std::vector<SDL_Thread*> threads;
    SDL_Mutex* gameMtx;
    SDL_Condition* gameCond;
    int pendingWorkCount;
    bool shouldExit; // Flag to signal threads to exit
};

#endif // WORKER_CONCURRENT
