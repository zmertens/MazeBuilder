#include "distances.h"

#include "cell.h"

using namespace mazes;
using namespace std;

distances::distances(std::shared_ptr<cell> root) : m_root(m_root) {
	m_cells[root] = 0;
}

void distances::set(std::shared_ptr<cell> cell, int distance) {
    m_cells[cell] = distance;
}

std::vector<std::shared_ptr<cell>> distances::get_cells() const {
    std::vector<std::shared_ptr<cell>> keys;
    for (const auto& pair : m_cells) {
        keys.push_back(pair.first);
    }
    return keys;
}

/**
 * @brief Compute the path to a goal cell
 *  Note the different usage of `at` and `[]` in the array accessors below:
 *  '[]' uses the m_cells operator override
 */
const std::shared_ptr<distances>& distances::path_to(std::shared_ptr<cell> goal) const noexcept {
    auto&& current = goal;
    auto breadcrumbs = std::make_shared<distances>(m_root);
    breadcrumbs->set(current, m_cells.at(current));

    while (current != m_root) {
        for (auto&& [neighbor, count] : current->get_links()) {
            if (m_cells.at(neighbor) < m_cells.at(current)) {
                breadcrumbs->set(neighbor, m_cells.at(neighbor));
                current = neighbor;
                break;
            }
        }
    }
    return breadcrumbs;
}

std::pair<std::shared_ptr<cell>, int> distances::max() const {
    int max_distance = 0;
    std::shared_ptr<cell> max_cell = m_root;
    for (const auto& pair : m_cells) {
        if (pair.second > max_distance) {
            max_cell = pair.first;
            max_distance = pair.second;
        }
    }
    return { max_cell, max_distance };
}