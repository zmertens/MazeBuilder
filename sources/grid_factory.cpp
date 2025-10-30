#include <MazeBuilder/grid_factory.h>

// #include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/string_utils.h>
#include <MazeBuilder/grid.h>

#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace mazes;

bool grid_factory::register_creator(const std::string &key, factory_creator_t creator) noexcept
{

    if (key.empty() || !creator)
    {

        return false;
    }

    std::lock_guard<std::mutex> lock(m_creators_mutex);

    // Check if key already exists
    if (m_creators.find(key) != m_creators.end())
    {

        return false;
    }

    m_creators.insert_or_assign(key, std::move(creator));

    return true;
}

bool grid_factory::unregister_creator(const std::string &key) noexcept
{

    std::lock_guard<std::mutex> lock(m_creators_mutex);

    if (auto it{m_creators.find(key)}; it != m_creators.cend())
    {

        m_creators.erase(it);

        return true;
    }

    return false;
}

bool grid_factory::is_registered(const std::string &key) const
{

    std::lock_guard<std::mutex> lock(m_creators_mutex);

    return m_creators.find(key) != m_creators.end();
}

std::optional<std::unique_ptr<grid_interface>> grid_factory::create(const std::string &key, const configurator &config) const noexcept
{

    std::lock_guard<std::mutex> lock(m_creators_mutex);

    if (auto it = m_creators.find(key); it != m_creators.cend())
    {
        try
        {
            auto g = it->second(config);

            if (!g)
            {

                throw new std::runtime_error(string_utils::format("Creator for key {} returned nullptr", key));
            }

            return g;
        }
        catch (const std::exception &ex)
        {

            std::cout << string_utils::format("Create grid failed with error message {}, is key registered? {}", ex.what(), std::to_string(is_registered(key))) << std::endl;
        }
        catch (...)
        {

            std::cout << "Create grid failed with unknown error, is key registered? {}" << std::to_string(is_registered(key)) << std::endl;
        }
    } // if

    return std::nullopt;
}

std::vector<std::string> grid_factory::get_registered_keys() const
{
    std::lock_guard<std::mutex> lock(m_creators_mutex);

    std::vector<std::string> keys;

    keys.reserve(m_creators.size());

    for (const auto &pair : m_creators)
    {

        keys.push_back(pair.first);
    }

    return keys;
}

void grid_factory::clear() noexcept
{

    std::lock_guard<std::mutex> lock(m_creators_mutex);

    m_creators.clear();
}
