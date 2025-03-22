#include <MazeBuilder/distances.h>

#include <iterator>
#include <MazeBuilder/cell.h>

using namespace mazes;
using namespace std;

distances::distances(std::shared_ptr<cell> root) : m_root(root), m_cells({}) {
    const auto& links = root->get_links();
    copy(links.cbegin(), links.cend(), inserter(m_cells, m_cells.end()));
    // Distance from root to root is 0
	m_cells.insert_or_assign(root, 0);
}

void distances::set(std::shared_ptr<cell> cell, int distance) noexcept {
    m_cells.insert_or_assign(cell, distance);
}

bool distances::contains(const std::shared_ptr<cell>& cell) const noexcept {
	return m_cells.find(cell) != m_cells.cend();
}

/**
 * @brief Compute the path to a goal cell
 */
std::shared_ptr<distances> distances::path_to(std::shared_ptr<cell> goal) const noexcept {
    auto path = std::make_shared<distances>(m_root);
    auto current = goal;

    while (current != goal) {
        if (!contains(current)) {
            return nullptr;
        }
        path->m_cells[current] = m_cells.at(current);
        auto neighbors = current->get_neighbors();

        // Filter neighbors to only those that are linked and contained in distances
        std::vector<std::shared_ptr<cell>> valid_neighbors;
        for (const auto& neighbor : neighbors) {
            if (contains(neighbor) && current->is_linked(neighbor)) {
                valid_neighbors.push_back(neighbor);
            }
        }

        if (valid_neighbors.empty()) {
            return nullptr;
        }

        // Find the neighbor with the minimum distance
        current = *std::min_element(valid_neighbors.begin(), valid_neighbors.end(),
            [this](const std::shared_ptr<cell>& a, const std::shared_ptr<cell>& b) {
                return m_cells.at(a) < m_cells.at(b);
            });
    }

    path->m_cells[m_root] = 0;
    return path;
}

std::pair<std::shared_ptr<cell>, int> distances::max() const noexcept {
    int max_distance = 0;
    std::shared_ptr<cell> max_cell = m_root;
    for (const auto& [c, d] : m_cells) {
        if (d > max_distance) {
            max_cell = c;
            max_distance = d;
        }
    }
    return { max_cell, max_distance };
}

void distances::collect_keys(std::vector<std::shared_ptr<cell>>& cells) const noexcept {
    for (const auto& [c, _] : m_cells) {
        cells.push_back(c);
    }
}
