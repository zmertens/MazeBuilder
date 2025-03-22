#include <MazeBuilder/cell.h>

using namespace mazes;
using namespace std;

cell::cell(std::int32_t index)
: m_index{ index }
, m_links{}
, m_north{ nullptr }
, m_south{ nullptr }
, m_east{ nullptr }
, m_west{ nullptr }  {

}

bool cell::has_key(const shared_ptr<cell>& c) const {
    return m_links.find(c) != m_links.end();
}

/// @brief 
/// @param c1 
/// @param c2 
/// @param bidi true
void cell::link(shared_ptr<cell> c1, shared_ptr<cell> c2, bool bidi) {
    this->m_links.insert_or_assign(c2, true);
    if (bidi) {
        c2->link(c2, c1, false);
    }
}

/// @brief 
/// @param c1 
/// @param c2 
/// @param bidi true
void cell::unlink(shared_ptr<cell> c1, shared_ptr<cell> c2, bool bidi) {
    this->m_links.erase(c1);
    if (bidi) {
        c2->unlink(c2, c1, false);
    }
}

unordered_map<shared_ptr<cell>, bool> cell::get_links() const {
    return this->m_links;
}

bool cell::is_linked(const shared_ptr<cell>& c) const {
    return has_key(c);
}

bool cell::has_northern_neighbor() const noexcept {
    return this->is_linked(this->m_north);
}

bool cell::has_southern_neighbor() const noexcept {
    return this->is_linked(this->m_south);
}

bool cell::has_eastern_neighbor() const noexcept {
    return this->is_linked(this->m_east);
}

bool cell::has_western_neighbor() const noexcept {
    return this->is_linked(this->m_west);
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

int cell::get_index() const {
    return this->m_index;
}

void cell::set_index(int next_index) noexcept {
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
