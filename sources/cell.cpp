#include <MazeBuilder/cell.h>

#include <mutex>

using namespace mazes;

/// @brief 
/// @param index 0
cell::cell(std::int32_t index)
    : m_index{ index }
    , m_links{} {

}

// Destructor
cell::~cell() {

    std::unique_lock<std::shared_mutex> lock(m_links_mutex);

    m_links.clear();
}

// Copy Constructor
cell::cell(const cell& other)
    : m_index(other.m_index) {

    // Copy links
    std::unique_lock<std::shared_mutex> lock(other.m_links_mutex);

    m_links = other.m_links;
}

// Copy Assignment Operator
cell& cell::operator=(const cell& other) {

    if (this == &other) {

        return *this;
    }

    // Copy index
    m_index = other.m_index;

    // Copy links
    {
        std::scoped_lock lock(m_links_mutex, other.m_links_mutex);

        m_links = other.m_links;
    }

    return *this;
}

// Move Constructor
cell::cell(cell&& other) noexcept
    : m_index(other.m_index) {

    // Move links
    std::unique_lock<std::shared_mutex> lock(other.m_links_mutex);

    m_links = std::move(other.m_links);
}

// Move Assignment Operator
cell& cell::operator=(cell&& other) noexcept {
    if (this == &other) {

        return *this;
    }

    // Move index
    m_index = other.m_index;

    // Move links
    {
        std::scoped_lock lock(m_links_mutex, other.m_links_mutex);

        m_links = std::move(other.m_links);
    }

    return *this;
}

bool cell::has_key(const std::shared_ptr<cell>& c) {

    std::lock_guard<std::shared_mutex> lock(m_links_mutex);

    for (const auto& [weak_cell, _] : m_links) {
        if (auto shared_cell = weak_cell.lock()) {
            if (shared_cell == c) {
                return true;
            }
        }
    }
    return false;
}

void cell::cleanup_links() {
    // Create a temporary map to hold non-expired links
    std::unordered_map<std::weak_ptr<cell>, bool, weak_ptr_hash, weak_ptr_equal> valid_links;

    {
        std::unique_lock<std::shared_mutex> lock(m_links_mutex);

        // Identify and collect non-expired links
        for (const auto& pair : m_links) {
            if (!pair.first.expired()) {
                valid_links.insert(pair);
            }
        }

        // If we have fewer valid links than total links, replace the map
        if (valid_links.size() < m_links.size()) {
            m_links = std::move(valid_links);
        }
    }
}

void cell::add_link(const std::shared_ptr<cell>& other) {
    if (!other) {

        return;
    }

    std::unique_lock<std::shared_mutex> lock(m_links_mutex);
    m_links.insert_or_assign(std::weak_ptr<cell>(other), true);

    // Instead of calling cleanup_links which would require another lock,
    // we'll remove any expired links while we already have the lock
    for (auto it = m_links.begin(); it != m_links.end(); ) {
        if (it->first.expired()) {
            it = m_links.erase(it);
        } else {
            ++it;
        }
    }
}

void cell::remove_link(const std::shared_ptr<cell>& other) {
    if (!other) {

        return;
    }

    std::unique_lock<std::shared_mutex> lock(m_links_mutex);
    m_links.erase(std::weak_ptr<cell>(other));

    // Clean up any expired links while we have the lock
    for (auto it = m_links.begin(); it != m_links.end(); ) {
        if (it->first.expired()) {
            it = m_links.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<std::pair<std::shared_ptr<cell>, bool>> cell::get_links() const {
    std::vector<std::pair<std::shared_ptr<cell>, bool>> shared_links;

    std::shared_lock<std::shared_mutex> lock(m_links_mutex);
    shared_links.reserve(m_links.size());

    for (const auto& [weak_cell, linked] : m_links) {
        if (auto shared_cell = weak_cell.lock()) {
            shared_links.emplace_back(shared_cell, linked);
        }
    }

    return shared_links;
}

bool cell::is_linked(const std::shared_ptr<cell>& c) {

    return has_key(c);
}

std::int32_t cell::get_index() const noexcept {
    return this->m_index;
}

void cell::set_index(std::int32_t next_index) noexcept {
    this->m_index = next_index;
}
