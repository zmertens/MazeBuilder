#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/async_logger.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

using namespace mazes;

TEST_CASE("async_logger delivers queued messages", "[async_logger]")
{
    std::mutex mtx;
    std::condition_variable cv;
    std::vector<std::string> received;

    async_logger logger([&](std::string_view msg)
                        {
                            std::lock_guard<std::mutex> lock(mtx);
                            received.emplace_back(msg);
                            cv.notify_one();
                        });

    logger.log("maze {}", 42);

    std::unique_lock<std::mutex> lock(mtx);
    const bool ok = cv.wait_for(lock, std::chrono::milliseconds(500), [&]()
                                { return !received.empty(); });

    REQUIRE(ok);
    REQUIRE(received.front() == "maze 42");

    logger.flush();
}
