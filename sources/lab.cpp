#include <MazeBuilder/lab.h>

#include <sstream>
#include <random>

#include <MazeBuilder/stringz.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/maze.h>

using namespace mazes;

/// @brief 
lab::lab() {

}

lab::~lab() = default;

lab::lab(const lab& other)
: m_p_q(other.m_p_q) {

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

