#include <MazeBuilder/lab.h>


#include <MazeBuilder/cell.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/maze.h>
#include <MazeBuilder/string_view_utils.h>

#include <random>
#include <sstream>

using namespace mazes;

/// @brief 
lab::lab()
: m_p_q{}
, levels{} {

}

lab::~lab() = default;

lab::lab(const lab& other)
: m_p_q(other.m_p_q)
, levels{} {

}

lab& lab::operator=(const lab& other) {
    if (this == &other) {
        return *this;
    }
    m_p_q = other.m_p_q;
    return *this;
}

std::optional<std::tuple<int, int, int, int>> lab::find(int p, int q) const noexcept {
    auto itr = m_p_q.find({ p, q });
    return (itr != m_p_q.cend()) ? make_optional(itr->second) : std::nullopt;
}

void lab::insert(int p, int q, int r, int w) noexcept {
    m_p_q.insert_or_assign({ p, q }, std::make_tuple(p, q, r, w));
}

bool lab::empty() const noexcept {
    return m_p_q.empty();
}

int lab::get_levels() const noexcept {
    return levels;
}

void lab::set_levels(int levels) noexcept {
    this->levels = levels;
}

int lab::get_random_block_id() const noexcept {
    using namespace std;

    mt19937 mt{ 42681ul };
    auto get_int = [&mt](int low, int high) {
        uniform_int_distribution<int> dist{ low, high };
        return dist(mt);
        };

    return get_int(0, 23);
}


void lab::link(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, bool bidi) noexcept {
    if (!c1 || !c2) return;

    // Add c2 to c1's links
    c1->add_link(c2);

    // If bidirectional, add c1 to c2's links
    if (bidi) {
        c2->add_link(c1);
    }
}

void lab::unlink(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, bool bidi) noexcept {
    if (!c1 || !c2) return;

    // Remove c2 from c1's links
    c1->remove_link(c2);

    // If bidirectional, remove c1 from c2's links
    if (bidi) {
        c2->remove_link(c1);
    }
}
