#ifndef CREATE_2_H
#define CREATE_2_H

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create.h>

#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

/// @file create2.h
/// @namespace mazes
namespace mazes
{
    /// @namespace detail
    namespace detail
    {
        /// @brief A worker class for concurrent maze generation
        class worker_concurrent
        {
        private:
            /// @brief A work item for maze generation
            struct work_item
            {
                unsigned int id;

                std::string work_str; // This will store the result of create() calls

                std::vector<configurator> configs; // Store the configs assigned to this worker

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

            std::string *target_str_ptr; // Pointer to the target string

            std::mutex target_str_mutex; // Mutex to protect target_str access

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

            void generate(const std::vector<configurator> &configs, std::string &target_str) noexcept
            {
                using namespace std;

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
                work_cond.wait(lock, [&]
                               { return pending_work_count.load() <= 0; });
            }

            void do_work(work_item &item) noexcept
            {
                using namespace std;

                for (const auto &config : item.configs)
                {
                    item.work_str += create(config);
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
    } // namespace details

    std::string create2(const std::vector<configurator> &configs)
    {
        if (configs.empty())
        {
            return {};
        }

        // For single config, just use the existing create function
        if (configs.size() == 1)
        {
            return create(configs[0]);
        }

        // Static worker pool for thread reuse
        static detail::worker_concurrent foreman{};
        static std::once_flag init_flag;
        
        // Initialize threads only once
        std::call_once(init_flag, [&]() {
            foreman.initThreads();
        });

        std::string result{};

        foreman.generate(configs, result);

        foreman.wait_for_completion();

        // Note: We don't call cleanup() here to keep threads alive for reuse

        return result;
    }

} // namespace mazes

#endif // CREATE_2_H