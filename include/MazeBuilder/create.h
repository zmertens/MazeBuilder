#ifndef CREATE_H
#define CREATE_H

#include <future>
#include <mutex>
#include <string>
#include <vector>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/maze_factory.h>
#include <MazeBuilder/maze_interface.h>
#include <MazeBuilder/maze_str.h>
#include <MazeBuilder/pixels.h>
#include <MazeBuilder/progress.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/stringify.h>
#include <MazeBuilder/string_utils.h>

#include <cmath>
#include <filesystem>

/// @file create.h
/// @namespace mazes
namespace mazes
{
    /// @namespace detail
    namespace detail
    {
        // Internal implementation - not intended for direct use
        static std::string create_single(const configurator& config)
        {
            auto grid_creator = [](const configurator& c) -> std::unique_ptr<grid_interface>
            {
                return std::make_unique<grid>(c.rows(), c.columns(), c.levels());
            };

            auto maze_creator = [grid_creator](const configurator& c) -> std::unique_ptr<maze_interface>
            {
                static constexpr auto GRID_CREATION_ID{"g1"};

                grid_factory gf{};

                if (!gf.is_registered(GRID_CREATION_ID))
                {
                    if (!gf.register_creator(GRID_CREATION_ID, grid_creator))
                    {
                        return nullptr;
                    }
                }

                if (const auto ptr = gf.create(GRID_CREATION_ID, std::cref(c));
                    ptr.has_value())
                {
                    thread_local randomizer rng{};

                    rng.seed(c.seed());

                    if (const auto runner = configurator::make_algo_from_config(std::cref(c));
                        runner.has_value())
                    {
                        const auto& val = ptr.value();

                        if (const auto success = runner.value()->run(val.get(), std::ref(rng)); success)
                        {
                            thread_local stringify stringifier;

                            stringifier.run(val.get(), std::ref(rng));

                            return std::make_unique<maze_str>(val->operations().get_str());
                        }
                    }
                }

                return nullptr;
            };

            std::string s{};

            [[maybe_unused]]
                auto x = progress<>::duration([&config, &s, maze_creator]() -> bool
                {
                    static constexpr auto MAZE_CREATION_ID{"m1"};

                    maze_factory mf{};

                    if (!mf.is_registered(MAZE_CREATION_ID))
                    {
                        if (!mf.register_creator(MAZE_CREATION_ID, maze_creator))
                        {
                            return false;
                        }
                    }

                    if (const auto maze_optional = mf.create(MAZE_CREATION_ID, std::cref(config));
                        maze_optional.has_value())
                    {
                        s = maze_optional.value()->maze();
                    }

                    return !s.empty();
                });

            return s;
        }

        // Smart concurrency execution based on hardware capabilities
        template <typename... Configs>
        static std::vector<std::string> create_async(const Configs&... configs)
        {
            static_assert(sizeof...(configs) > 1, "Need multiple configs for concurrent execution");

            const unsigned int hardware_threads = std::thread::hardware_concurrency();
            const size_t num_configs = sizeof...(configs);

            std::vector<std::string> results;
            results.reserve(num_configs);

            if (hardware_threads > 1 && num_configs >= 2)
            {
                // Use async execution for multi-core systems
                std::vector<std::future<std::string>> futures;
                futures.reserve(num_configs);

                // Launch async tasks for each configuration
                auto launch_async = [](const configurator& config)
                {
                    return std::async(std::launch::async, [config]()
                    {
                        return create_single(config);
                    });
                };

                (futures.push_back(launch_async(configs)), ...);

                // Collect results
                for (auto& future : futures)
                {
                    results.push_back(future.get());
                }
            }
            else
            {
                // Fall back to serial execution for single-core or small workloads
                (results.push_back(create_single(configs)), ...);
            }

            return results;
        }
    } // namespace detail

    // Public API: Create maze from single configurator
    template <typename Config>
    static std::string create(const Config& config)
    {
        // Handle both direct configurator and reference_wrapper
        using DecayedConfig = std::decay_t<Config>;

        static_assert(
            std::is_same_v<DecayedConfig, configurator> ||
            std::is_same_v<DecayedConfig, std::reference_wrapper<configurator>> ||
            std::is_same_v<DecayedConfig, std::reference_wrapper<const configurator>>,
            "Parameter must be a configurator or reference to configurator");

        // Handle reference wrapper by getting the actual configurator
        if constexpr (std::is_same_v<DecayedConfig, std::reference_wrapper<configurator>> ||
            std::is_same_v<DecayedConfig, std::reference_wrapper<const configurator>>)
        {
            return detail::create_single(config.get());
        }
        else
        {
            return detail::create_single(config);
        }
    }

    // Public API: Create multiple mazes from multiple configurators
    template <typename... Configs>
    static std::vector<std::string> create(const Configs&... configs)
    {
        static_assert(sizeof...(configs) > 1, "Use single parameter version for one configurator");

        // Handle both direct configurators and reference wrappers
        static_assert(
            ((std::is_same_v<std::decay_t<Configs>, configurator> ||
                    std::is_same_v<std::decay_t<Configs>, std::reference_wrapper<configurator>> ||
                    std::is_same_v<std::decay_t<Configs>, std::reference_wrapper<const configurator>>) &&
                ...),
            "All parameters must be configurator objects or references to configurators");

        // Process each configurator and unwrap reference wrappers if needed
        auto unwrap_config = []<typename Config>(const Config& cfg) -> const configurator&
        {
            using ConfigType = std::decay_t<Config>;
            if constexpr (std::is_same_v<ConfigType, std::reference_wrapper<configurator>> ||
                std::is_same_v<ConfigType, std::reference_wrapper<const configurator>>)
            {
                return cfg.get();
            }
            else
            {
                return cfg;
            }
        };

        // Use smart concurrency execution
        return detail::create_async(unwrap_config(configs)...);
    }
} // namespace mazes

#endif // CREATE_H
