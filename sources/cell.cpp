#include "cell.h"

#include <queue>

#include "distances.h"

using namespace mazes;
using namespace std;

cell::cell(int index)
: m_row{ 0 }
, m_column{ 0 }
, m_index{ index }
, m_links{}
, m_north{ nullptr }
, m_south{ nullptr }
, m_east{ nullptr }
, m_west{ nullptr }
, m_left{ nullptr }
, m_right{}  {

}

cell::cell(unsigned int row, unsigned int column, int index) 
: m_row{row}
, m_column{column}
, m_index{index}
, m_links{}
, m_north{nullptr}
, m_south{nullptr}
, m_east{nullptr}
, m_west{nullptr}
, m_left{ nullptr }
, m_right{} {

}

bool cell::has_key(const shared_ptr<cell>& c) const {
    return m_links.find(c) != m_links.end();
}

/**
 * @param bidi = true
*/
void cell::link(shared_ptr<cell> c1, shared_ptr<cell> c2, bool bidi) {
    this->m_links.insert_or_assign(c2, true);
    if (bidi) {
        c2->link(c2, c1, false);
    }
}

/**
 * @param bidi = true
*/
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

unsigned int cell::get_row() const {
    return this->m_row;
}

unsigned int cell::get_column() const {
    return this->m_column;
}

int cell::get_index() const {
    return this->m_index;
}

void cell::set_index(int next_index) noexcept {
    this->m_index = next_index;
}

void cell::set_color(std::uint32_t c) noexcept {
	this->m_color = c;
}

std::uint32_t cell::get_color() const noexcept {
	return this->m_color;
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

std::shared_ptr<cell> cell::get_left() const {
    return this->m_left;
}

std::shared_ptr<cell> cell::get_right() const {
    return this->m_right;
}

void cell::set_left(std::shared_ptr<cell> const& other_left) {
    this->m_left = other_left;
}

void cell::set_right(std::shared_ptr<cell> const& other_right) {
    this->m_right = other_right;
}

void cell::set_row(unsigned int r) noexcept {
    this->m_row = r;
}

void cell::set_column(unsigned int c) noexcept {
    this->m_column = c;
}

std::shared_ptr<distances> cell::get_distances_from(const shared_ptr<cell>& root) noexcept {
    shared_ptr<distances> dists = make_shared<distances>( root );

    std::priority_queue<std::pair<int, std::shared_ptr<cell>>, std::vector<std::pair<int, std::shared_ptr<cell>>>, std::greater<>> frontier;
    frontier.push({ 0, root });

    while (!frontier.empty()) {
        auto [current_distance, current_cell] = frontier.top();
        frontier.pop();

        for (const auto& neighbor : current_cell->get_neighbors()) {
            // Assuming each edge has a weight of 1
            int new_distance = current_distance + 1;
            if (!dists->contains(neighbor) || new_distance < (*dists)[neighbor]) {
                (*dists)[neighbor] = new_distance;
                frontier.push({ new_distance, neighbor });
            }
        }
    }

    return dists;
}