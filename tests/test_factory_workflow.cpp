#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/maze_factory.h>
#include <MazeBuilder/maze_str.h>
#include <MazeBuilder/progress.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/stringify.h>
#include <MazeBuilder/string_utils.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

using namespace mazes;
using namespace std;

static constexpr auto ROWS = 10, COLUMNS = 5, LEVELS = 1;

static constexpr auto ALGO_DFS = algo::DFS;

static constexpr auto SEED = 12345;

// Provides standard out in async context
struct pcout : public stringstream
{
    static inline mutex cout_mutex;

    ~pcout()
    {
        lock_guard<mutex> l{cout_mutex};
        cout << rdbuf();
        cout.flush();
    }
};

// Helper function to create maze and return str
static string create(const configurator &config)
{
    auto grid_creator = [](const configurator &config) -> std::unique_ptr<grid_interface>
    {
        return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
    };

    auto maze_creator = [grid_creator](const configurator &config) -> std::unique_ptr<maze_interface>
    {
        if (config.algo_id() != algo::DFS)
        {
            return nullptr;
        }

        grid_factory gf{};

        if (!gf.is_registered("g1"))
        {

            REQUIRE(gf.register_creator("g1", grid_creator));
        }

        if (auto igrid = gf.create("g1", cref(config)); igrid.has_value())
        {
            static dfs _dfs{};

            static randomizer rng{};

            rng.seed(config.seed());

            auto &&igridimpl = igrid.value();

            if (auto success = _dfs.run(igridimpl.get(), ref(rng)))
            {
                static stringify _stringifier;

                _stringifier.run(igridimpl.get(), ref(rng));

                return make_unique<maze_str>(igridimpl->operations().get_str());
            }
        }

        return nullptr;
    };

    string s{};

    auto duration = progress<>::duration([&config, maze_creator, &s]() -> bool
                                         {

        maze_factory mf{};

        if (!mf.is_registered("custom_maze")) {

            REQUIRE(mf.register_creator("custom_maze", maze_creator));
        }

        auto maze_optional = mf.create("custom_maze", cref(config));

        REQUIRE(maze_optional.has_value());

        s = maze_optional.value()->maze();
        
        return !s.empty(); });

    // pcout{} << duration.count() << " ms" << endl;

    return s;
}

static string concat(const string &a, const string &b)
{

    return a + b;
}

template <typename F>
static auto asynchronize(F f)
{
    return [f](auto... xs)
    {
        return [=]()
        {
            return async(launch::async, f, xs...);
        };
    };
}

template <typename F>
static auto fut_unwrap(F f)
{
    return [f](auto... xs)
    {
        return f(xs.get()...);
    };
}

template <typename F>
static auto async_adapter(F f)
{
    return [f](auto... xs)
    {
        return [=]()
        {
            return async(launch::async, fut_unwrap(f), xs()...);
        };
    };
}

class worker_concurrent
{
private:
    struct work_item
    {

        unsigned int id;

        std::string work_str;  // This will store the result of create() calls

        std::vector<configurator> configs;  // Store the configs assigned to this worker

        int start, count;

        work_item(unsigned int id, std::vector<configurator> configs, int start, int count)
            : id{id}, work_str{}, configs(std::move(configs)), start{start}, count{count}
        {
        }
    };

    std::condition_variable work_cond;

    std::mutex work_mtx;

    std::atomic<int> pending_work_count;

    std::atomic<bool> should_exit;

    std::deque<work_item> work_queue;

    std::vector<std::thread> workers;

    std::string* target_str_ptr;  // Pointer to the target string

    std::mutex target_str_mutex;  // Mutex to protect target_str access

public:
    // Add constructor and destructor
    worker_concurrent() : should_exit(false), pending_work_count(0), target_str_ptr(nullptr) {}

    ~worker_concurrent()
    {

        cleanup();
    }

    // Initialize worker threads
    void initThreads() noexcept
    {

        using namespace std;

        auto thread_func = [](worker_concurrent *worker_ptr)
        {

            while (!worker_ptr->should_exit.load())
            {
                unique_lock<mutex> lock(worker_ptr->work_mtx);

                worker_ptr->work_cond.wait(lock, [worker_ptr]
                                           { return worker_ptr->should_exit.load() || !worker_ptr->work_queue.empty(); });

                if (worker_ptr->should_exit.load())
                {

                    break;
                }

                // Ready to do work
                if (!worker_ptr->work_queue.empty())
                {

                    auto temp_worker = std::move(worker_ptr->work_queue.front());

                    worker_ptr->work_queue.pop_front();

                    lock.unlock();

                    worker_ptr->do_work(temp_worker);

                    lock.lock();

                    if (--worker_ptr->pending_work_count <= 0)
                    {
                        // pcout{} << string_utils::format("{}", target_str);  
                        worker_ptr->work_cond.notify_one();
                    }
                }
            } // while
        }; // lambda

        static constexpr auto NUM_WORKERS = 4;

        // Create worker threads
        for (auto w{0}; w < NUM_WORKERS; w++)
        {

            workers.emplace_back(thread_func, this);
        }
    }

    void generate(const std::vector<configurator> &configs, std::string& target_str) noexcept
    {

        using namespace std;

        if (configs.empty())
        {

            return;
        }

        // Set the target string pointer
        target_str_ptr = &target_str;

        {
            unique_lock<std::mutex> lock(this->work_mtx);

            work_queue.clear();

            static constexpr auto BLOCK_COUNT = 4;

            size_t items_per_worker = configs.size() / BLOCK_COUNT;
            size_t remaining_items = configs.size() % BLOCK_COUNT;

            size_t current_index = 0;

            for (auto w{0}; w < BLOCK_COUNT; w++)
            {
                size_t start_index = current_index;
                
                // Distribute items evenly, with remainder distributed to first workers
                size_t count = items_per_worker + (w < remaining_items ? 1 : 0);
                
                if (count > 0)
                {
                    // Extract configs for this worker
                    std::vector<configurator> worker_configs;
                    for (size_t i = start_index; i < start_index + count && i < configs.size(); ++i)
                    {
                        worker_configs.push_back(configs[i]);
                    }

                    work_queue.emplace_back(static_cast<unsigned int>(w), std::move(worker_configs), static_cast<int>(start_index), static_cast<int>(count));
                }

                current_index += count;
            }

            this->pending_work_count.store(static_cast<int>(work_queue.size()));
        }

        this->work_cond.notify_all();
    } // generate

    void wait_for_completion() noexcept
    {
        using namespace std;
        
        unique_lock<mutex> lock(work_mtx);
        work_cond.wait(lock, [&] { return pending_work_count.load() <= 0; });
    }

    void do_work(work_item &item) noexcept
    {
        using namespace std;

        for (const auto& config : item.configs)
        {
            item.work_str += create(cref(config));
        }

        // Append the worker's result to the shared target string
        if (target_str_ptr && !item.work_str.empty())
        {
            lock_guard<mutex> lock(target_str_mutex);
            *target_str_ptr += item.work_str;
        }
    }

    void cleanup() noexcept
    {
        {
            std::lock_guard<std::mutex> lock(work_mtx);

            should_exit = true;

            pending_work_count = 0;
        }

        work_cond.notify_all();

        for (auto &t : workers)
        {

            if (t.joinable())
            {

                t.join();
            }
        }

        workers.clear();
    }
};

TEST_CASE("Workflow static checks", "[workflow_static_checks]")
{

    STATIC_REQUIRE(std::is_default_constructible<mazes::grid_factory>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::grid_factory>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible<mazes::grid_factory>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable<mazes::grid_factory>::value);
    STATIC_REQUIRE_FALSE(std::is_move_constructible<mazes::grid_factory>::value);
    STATIC_REQUIRE_FALSE(std::is_move_assignable<mazes::grid_factory>::value);

    STATIC_REQUIRE(std::is_default_constructible<mazes::maze_factory>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::maze_factory>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible<mazes::maze_factory>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable<mazes::maze_factory>::value);
    STATIC_REQUIRE_FALSE(std::is_move_constructible<mazes::maze_factory>::value);
    STATIC_REQUIRE_FALSE(std::is_move_assignable<mazes::maze_factory>::value);

    STATIC_REQUIRE(std::is_default_constructible<mazes::randomizer>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::randomizer>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::randomizer>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::randomizer>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::randomizer>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::randomizer>::value);
}

TEST_CASE("Test grid_factory create1", "[create1]")
{

    grid_factory factory1{};

    static const string PRODUCT_NAME_1 = "test_grid";

    // Register a custom creator
    factory1.register_creator(PRODUCT_NAME_1, [](const configurator &config) -> std::unique_ptr<grid_interface>
                              { return std::make_unique<grid>(config.rows(), config.columns(), config.levels()); });

    // Create using the registered key
    REQUIRE(factory1.create(PRODUCT_NAME_1, configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_DFS).seed(SEED)) != std::nullopt);
}

TEST_CASE("Test full workflow", "[full workflow]")
{

    grid_factory g_factory{};

    auto key{"key"};

    g_factory.register_creator(key, [](const configurator &config) -> std::unique_ptr<grid_interface>
                               { return std::make_unique<grid>(config.rows(), config.columns(), config.levels()); });

    auto g = g_factory.create(key, configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_DFS).seed(SEED));

    randomizer rndmzr{};

    stringify stringifier{};

    REQUIRE(stringifier.run(g.value().get(), rndmzr));

    if (auto casted_grid = dynamic_cast<grid *>(g.value().get()); casted_grid != nullptr)
    {

        REQUIRE_FALSE(casted_grid->operations().get_str().empty());
    }
    else
    {

        FAIL("Failed to cast to grid");
    }
}

TEST_CASE("Test full workflow with large grid", "[full workflow][large]")
{

    grid_factory g_factory{};

    auto key{"key"};

    g_factory.register_creator(key, [](const configurator &config) -> std::unique_ptr<grid_interface>
                               { return std::make_unique<grid>(config.rows(), config.columns(), config.levels()); });

    auto g = g_factory.create(key, configurator()
                                       .rows(configurator::MAX_ROWS)
                                       .columns(configurator::MAX_COLUMNS)
                                       .levels(configurator::MAX_LEVELS)
                                       .algo_id(ALGO_DFS)
                                       .seed(SEED));

    // Verify the grid was created successfully
    REQUIRE(g.has_value());
}

TEST_CASE("Invalid args when converting algo string", "[invalid args]")
{

    vector<string> algos_to_convert = {"dfz", "BINARY_TREE", "adjacentwinder"};

    for (auto a : algos_to_convert)
    {
        REQUIRE_THROWS_AS(mazes::to_algo_from_string(cref(a)), std::invalid_argument);
    }
}

TEST_CASE("randomizer::get_num_ints generates correct number of integers", "[randomizer]")
{
    randomizer rng;
    rng.seed(SEED);

    static constexpr auto low = 0, high = 10;

    SECTION("Validate random number values are within specific range")
    {

        auto result = rng.get_vector_ints(low, high - 1, high);
        REQUIRE(result.size() == high);
        for (int num : result)
        {
            REQUIRE(num >= low);
            REQUIRE(num <= high);
        }
    }

    SECTION("Generate all integers in a range")
    {
        auto result = rng.get_vector_ints(low, high, 2);
        REQUIRE(result.size() == 2);
        std::sort(result.begin(), result.end());
        REQUIRE(result.cend() != result.cbegin());
    }

    SECTION("Empty range [high, low]")
    {
        auto result = rng.get_vector_ints(high, low);
        REQUIRE(result.empty());
    }

    SECTION("Zero integers requested")
    {
        auto result = rng.get_vector_ints(0, -1);
        REQUIRE(result.empty());
    }
}

TEST_CASE("Grid grid_factory registration", "[grid_factory registration]")
{

    grid_factory grid_factory{};

    SECTION("Can register custom creator")
    {
        auto custom_creator = [](const configurator &config) -> std::unique_ptr<grid_interface>
        {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
        };

        REQUIRE(grid_factory.register_creator("custom_grid", custom_creator));
        REQUIRE(grid_factory.is_registered("custom_grid"));

        // Cannot register same key twice
        REQUIRE_FALSE(grid_factory.register_creator("custom_grid", custom_creator));
    }

    SECTION("Can register custom creator with distances")
    {
        auto custom_creator = [](const auto &config) -> auto
        {
            return std::make_unique<distance_grid>(config.rows(), config.columns(), config.levels());
        };

        REQUIRE(grid_factory.register_creator("distance_grid", custom_creator));
        REQUIRE(grid_factory.is_registered("distance_grid"));

        // Cannot register same key twice
        REQUIRE_FALSE(grid_factory.register_creator("distance_grid", custom_creator));
    }

    SECTION("Can create grid using registered key")
    {
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED);

        auto grid = grid_factory.create("grid", config);
        REQUIRE(grid != nullptr);

        auto distance_grid = grid_factory.create("distance_grid", config);
        REQUIRE(distance_grid != nullptr);

        auto colored_grid = grid_factory.create("colored_grid", config);
        REQUIRE(colored_grid != nullptr);
    }

    SECTION("Create returns nullptr for unregistered key")
    {
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED);

        auto grid = grid_factory.create("non_existent_key", config);
        REQUIRE_FALSE(grid.has_value());
    }

    SECTION("Can unregister creator")
    {
        auto custom_creator = [](const configurator &config) -> std::unique_ptr<grid_interface>
        {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
        };

        REQUIRE(grid_factory.register_creator("temp_grid", custom_creator));
        REQUIRE(grid_factory.is_registered("temp_grid"));

        REQUIRE(grid_factory.unregister_creator("temp_grid"));
        REQUIRE_FALSE(grid_factory.is_registered("temp_grid"));

        // Cannot unregister non-existent key
        REQUIRE_FALSE(grid_factory.unregister_creator("temp_grid"));
    }

    SECTION("Backward compatibility - create with config only")
    {
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED);

        // Default behavior without distances
        auto grid1 = grid_factory.create("test", config);
        REQUIRE(grid1 != nullptr);

        // With distances but text output
        config.distances(true);
        auto grid2 = grid_factory.create("test", config);
        REQUIRE(grid2 != nullptr);

        // With distances and image output
        config.output_format_id(output_format::PNG);
        auto grid3 = grid_factory.create("test", config);
        REQUIRE(grid3 != nullptr);
    }

    SECTION("Clear removes all creators")
    {
        auto custom_creator = [](const configurator &config) -> std::unique_ptr<grid_interface>
        {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
        };

        grid_factory.register_creator("temp_grid", custom_creator);
        REQUIRE(grid_factory.is_registered("temp_grid"));

        grid_factory.clear();

        REQUIRE_FALSE(grid_factory.is_registered("temp_grid"));
    }
}

#if defined(MAZE_BENCHMARK)

// Source material here
// https://github.com/PacktPublishing/Cpp17-STL-Cookbook/blob/master/Chapter09/chains.cpp
TEST_CASE("Maze maze_factory registration with async", "[maze_factory async workflow]")
{
    configurator config1{};
    config1.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED).distances(true).algo_id(ALGO_DFS);

    configurator config2{};
    config2.rows(COLUMNS).columns(ROWS).levels(LEVELS).seed(SEED).distances(true).algo_id(ALGO_DFS);

    auto pcreate(asynchronize(create));
    auto pconcat(async_adapter(concat));

    auto result = pconcat(pcreate(cref(config2)), pconcat(pcreate(cref(config1)), pcreate(cref(config2))));

    BENCHMARK("Async concat")
    {
        // Use structured bindings to capture both the maze and timing
        auto [content, execution_time] = [&result]()
        {
            std::string maze_content;

            auto duration = progress<>::duration([&result, &maze_content]() -> bool
                                                 {
                                                 maze_content = result().get();
                                                 return !maze_content.empty(); });

            return std::make_tuple(std::move(maze_content), duration.count());
        }();

        REQUIRE_FALSE(content.empty());
        REQUIRE_FALSE(execution_time == 0);
    };

    BENCHMARK("Serial-executed create")
    {
        auto s1 = create(cref(config1));

        auto s2 = create(cref(config2));

        auto s3 = create(cref(config1));

        auto concatenated = concat(concat(s1, s2), s3);

        REQUIRE_FALSE(concatenated.empty());
    };

    worker_concurrent foreman{};

    foreman.initThreads();

    BENCHMARK("Worker threads execution")
    {
        std::vector<configurator> configs = {config1, config2, config1, config2, config1, config1, config2};

        std::string target_str{};

        foreman.generate(configs, target_str);

        foreman.wait_for_completion();
    };

    foreman.cleanup();
}
#endif // MAZE_BENCHMARK
