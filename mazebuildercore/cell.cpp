#include <MazeBuilder/cell.h>

#include <MazeBuilder/async_logger.h>

#include <algorithm>
#include <ranges>
#include <string>

using namespace mazes;

/// @brief
/// @param area {1u, u1}
cell::cell(std::pair<std::uint8_t, std::uint8_t> index)
    : m_area{index}
{
}

// Destructor
cell::~cell()
{
    cleanup_links();
}

// Copy Constructor
cell::cell(const cell &other)
    : m_area(other.m_area)
{
    m_links = other.m_links;
}

// Copy Assignment Operator
cell &cell::operator=(const cell &other)
{
    if (this == &other)
    {
        return *this;
    }

    // Copy index
    m_area = other.m_area;

    m_links = other.m_links;

    return *this;
}

// Move Constructor
cell::cell(cell &&other) noexcept
    : m_area(other.m_area)
{
    m_links = std::move(other.m_links);
}

// Move Assignment Operator
cell &cell::operator=(cell &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    // Move index
    m_area = other.m_area;

    m_links = std::move(other.m_links);

    return *this;
}

bool cell::has_key(const std::shared_ptr<cell> &c) const noexcept
{
    return std::ranges::any_of(m_links, [&c](const auto &pair)
                               {
            if (auto shared_cell = pair.first.lock())
            {
                return shared_cell == c;
            }
            return false; });
}

void cell::cleanup_links()
{
    [[maybe_unused]] const auto removed = std::erase_if(m_links, [](const auto &pair)
                                                        { return pair.first.expired(); });

#if defined(MAZE_DEBUG)
    async_logger::instance()->log("Cleaned " + std::to_string(removed) + " expired links");
#endif
}

void cell::add_link(const std::shared_ptr<cell> &other) noexcept
{
    if (!other)
    {
        return;
    }

    m_links.insert_or_assign(std::weak_ptr(other), true);

    // Instead of calling cleanup_links which would require another lock,
    // we'll remove any expired links while we already have the lock
    for (auto it = m_links.begin(); it != m_links.cend();)
    {
        if (it->first.expired())
        {
            it = m_links.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void cell::remove_link(const std::shared_ptr<cell> &other) noexcept
{
    if (!other)
    {
        return;
    }

    m_links.erase(std::weak_ptr(other));

    // Clean up any expired links while we have the lock
    for (auto it = m_links.begin(); it != m_links.cend();)
    {
        if (it->first.expired())
        {
            it = m_links.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void cell::clear_links() noexcept
{
    m_links.clear();
}

bool cell::is_linked(const std::shared_ptr<cell> &c) const noexcept
{
    return this->has_key(c);
}

std::pair<std::uint8_t, std::uint8_t> cell::get_area() const noexcept
{
    return this->m_area;
}

void cell::set_area(std::pair<std::uint8_t, std::uint8_t> new_area) noexcept
{
    this->m_area = new_area;
}
