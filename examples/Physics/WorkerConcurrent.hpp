#ifndef WORKER_CONCURRENT
#define WORKER_CONCURRENT

#include <deque>
#include <vector>
#include <string_view>

#include "State.hpp"

struct SDL_Thread;
struct SDL_Mutex;
struct SDL_Condition;
struct SDL_Vertex;

/// @brief Provides concurrent worker threads for string processing
/// @details This class manages a queue of work items and spawns multiple threads to process them concurrently.
/// @details Each thread processes a segment of a string, setting vertices for rendering.
class WorkerConcurrent {
public:
    explicit WorkerConcurrent(State& state);
    ~WorkerConcurrent();
    WorkerConcurrent(const WorkerConcurrent& other);
    WorkerConcurrent& operator=(const WorkerConcurrent& other);
    WorkerConcurrent(WorkerConcurrent&& other) noexcept;
    WorkerConcurrent& operator=(WorkerConcurrent&& other) noexcept;

    void initThreads() noexcept;
    void generate(std::string_view tab) noexcept;

private:
    struct WorkItem {
        const std::string_view& sv;
        std::vector<SDL_Vertex>& vertices;
        int start, count;
        int rows, columns;
        WorkItem(const std::string_view& sv,
            std::vector<SDL_Vertex>& vertices,
            int start, int count, int rows, int columns)
            : sv(sv)
            , vertices(vertices), start{ start }, count{ count }, rows{ rows }, columns{ columns } {

        }
    };

    void doWork(std::vector<SDL_Vertex>& vertices, WorkItem const& workItem) const noexcept;

    State& state;

    std::deque<WorkItem> workQueue;
    std::vector<SDL_Thread*> threads;
    SDL_Mutex* gameMtx;
    SDL_Condition* gameCond;
    int pendingWorkCount;
};

#endif // WORKER_CONCURRENT
