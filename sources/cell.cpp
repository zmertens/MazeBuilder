#include <MazeBuilder/cell.h>

using namespace mazes;
using namespace std;

/// @brief 
/// @param index 0
cell::cell(std::int32_t index)
: m_index{ index }
, m_links{}
, m_north{ nullptr }
, m_south{ nullptr }
, m_east{ nullptr }
, m_west{ nullptr }  {

}

bool cell::has_key(const shared_ptr<cell>& c) {
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

/// @brief 
/// @param other
/// @param bidi true
void cell::link(const std::shared_ptr<cell>& other, bool bidi) noexcept {
    if (other) {
        {
            std::lock_guard<std::shared_mutex> lock(m_links_mutex);
            m_links.insert_or_assign(std::weak_ptr<cell>(other), true);
        }

        if (bidi) {
            other->link(shared_from_this(), false);
        }
    }
}

/// @brief 
/// @param other 
/// @param bidi true
void cell::unlink(const std::shared_ptr<cell>& other, bool bidi) noexcept {
    if (other) {
        std::lock_guard<std::shared_mutex> lock(m_links_mutex);
        // Unlink this cell from the other cell
        m_links.erase(std::weak_ptr<cell>(other));

        // If bidirectional, unlink the other cell from this cell
        if (bidi) {
            other->unlink(shared_from_this(), false);
        }
    }
}

std::vector<std::pair<std::shared_ptr<cell>, bool>> cell::get_links() const {
    std::vector<std::pair<std::shared_ptr<cell>, bool>> shared_links;
    {
        std::lock_guard<std::shared_mutex> lock(m_links_mutex);
        for (const auto& [weak_cell, linked] : m_links) {
            if (auto shared_cell = weak_cell.lock()) {
                shared_links.emplace_back(shared_cell, linked);
            }
        }
    }
    return shared_links;
}

bool cell::is_linked(const shared_ptr<cell>& c) {
    return has_key(c);
}

bool cell::has_northern_neighbor() const noexcept {
    return nullptr != this->m_north;
}

bool cell::has_southern_neighbor() const noexcept {
    return nullptr != this->m_south;
}

bool cell::has_eastern_neighbor() const noexcept {
    return nullptr != this->m_east;
}

bool cell::has_western_neighbor() const noexcept {
    return nullptr != this->m_west;
}

vector<shared_ptr<cell>> cell::get_neighbors() const noexcept {
    vector<shared_ptr<cell>> neighbors;
    if (this->m_north)
        neighbors.emplace_back(this->m_north);
    if (this->m_south)
        neighbors.emplace_back(this->m_south);
    if (this->m_west)
        neighbors.emplace_back(this->m_west);
    if (this->m_east)
        neighbors.emplace_back(this->m_east);

    return neighbors;
}

std::int32_t cell::get_index() const noexcept {
    return this->m_index;
}

void cell::set_index(std::int32_t next_index) noexcept {
    this->m_index = next_index;
}

shared_ptr<cell> cell::get_north() const {
    return this->m_north;
}

shared_ptr<cell> cell::get_south() const {
    return this->m_south;
}

shared_ptr<cell> cell::get_east() const {
    return this->m_east;
}

shared_ptr<cell> cell::get_west() const {
    return this->m_west;
}

void cell::set_north(shared_ptr<cell> const& other) {
    this->m_north = other;
}

void cell::set_south(shared_ptr<cell> const& other) {
    this->m_south = other;
}

void cell::set_east(shared_ptr<cell> const& other) {
    this->m_east = other;
}

void cell::set_west(shared_ptr<cell> const& other) {
    this->m_west = other;
}
