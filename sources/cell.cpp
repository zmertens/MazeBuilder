#include "cell.h"

#include <iostream>

using namespace mazes;
using namespace std;

cell::cell(unsigned int row, unsigned int column, unsigned int index) 
: m_row{row}
, m_column{column}
, m_index{index}
, m_links{}
, m_north{nullptr}
, m_south{nullptr}
, m_east{nullptr}
, m_west{nullptr} {

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

unsigned int cell::get_index() const {
    return this->m_index;
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
