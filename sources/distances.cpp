#include <MazeBuilder/distances.h>

#include <iterator>
#include <MazeBuilder/cell.h>

using namespace mazes;
using namespace std;

distances::distances(std::shared_ptr<cell> root) : m_root(root), m_cells({}) {
    // Distance from root to root is 0
    m_cells.insert_or_assign(root, 0);

    // Initialize distances for linked cells
    const auto& links = root->get_links();
    for (const auto& [linked_cell, _] : links) {
        m_cells.insert_or_assign(linked_cell, 1); // Default distance for linked cells
    }
}

void distances::set(std::shared_ptr<cell> cell, int distance) noexcept {
    m_cells.insert_or_assign(cell, distance);
}

bool distances::contains(const std::shared_ptr<cell>& cell) const noexcept {
	return m_cells.find(cell) != m_cells.cend();
}

/// @brief Computes the shortest path to a goal cell within a distances object.
/// @param goal A shared pointer to the goal cell for which the path is to be computed.
/// @return A shared pointer to a distances object representing the path to the goal cell, or nullptr if the path cannot be found.
std::shared_ptr<distances> distances::path_to(std::shared_ptr<cell> goal) const noexcept {
    if (!goal || !contains(goal)) {

        return nullptr;
    }

    auto breadcrumbs = std::make_shared<distances>(m_root);
    auto current = goal;

    breadcrumbs->set(current, m_cells.at(current));

    while (current != m_root) {

        auto found{ false };

        auto neighbors = current->get_links();

        for (const auto& [neighbor, _] : neighbors) {
            if (m_cells.at(neighbor) < m_cells.at(current)) {
                breadcrumbs->set(neighbor, m_cells.at(neighbor));
                current = neighbor;
                found = true;
                break;
            }
        }

        if (!found) {
            return nullptr;
        }
    }

    return breadcrumbs;
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
