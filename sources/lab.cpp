#include <MazeBuilder/lab.h>

#include <sstream>

#include <MazeBuilder/stringz.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/maze.h>

using namespace mazes;

/// @brief 
lab::lab() {

}

lab::~lab() = default;

lab::lab(const lab& other)
: m_p_q(other.m_p_q)
, m_vertices(other.m_vertices)
, m_faces(other.m_faces) {

}

lab& lab::operator=(const lab& other) {
    if (this == &other) {
        return *this;
    }
    m_p_q = other.m_p_q;
    m_vertices = other.m_vertices;
    m_faces = other.m_faces;
    return *this;
}

std::optional<std::tuple<int, int, int, int>> lab::find(int p, int q) const noexcept {
    auto itr = m_p_q.find({ p, q });
    return (itr != m_p_q.cend()) ? make_optional(itr->second) : std::nullopt;
}

void lab::insert(int x, int y, int z, int w) noexcept {
    m_p_q[{x, z}] = std::make_tuple(x, y, z, w);
}

std::vector<std::tuple<int, int, int, int>> lab::get_render_vertices() const noexcept {
    using namespace std;

    vector<tuple<int, int, int, int>> render_vertices(this->m_vertices.size() / 8);
    for (size_t i = 0; i < this->m_vertices.size(); i += 8) {
        render_vertices.push_back(this->m_vertices[i]);
    }

    return render_vertices;
}
