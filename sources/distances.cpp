#include "distances.h"

#include "cell.h"

using namespace mazes;
using namespace std;

distances::distances(std::shared_ptr<cell> root) : m_root(root) {
	m_cells[root] = 0;
}

void distances::set(std::shared_ptr<cell> cell, int distance) {
    m_cells[cell] = distance;
}

std::vector<std::shared_ptr<cell>> distances::get_cells() const {
    std::vector<std::shared_ptr<cell>> keys;
	keys.reserve(m_cells.size());
    
	for (const auto& [cell, distance] : m_cells) {
		keys.push_back(cell);
	}

    return keys;
}

/**
 * @brief Compute the path to a goal cell
 *  Note the different usage of `at` and `[]` in the array accessors below:
 *  '[]' uses the m_cells operator override
 */
std::shared_ptr<distances> distances::path_to(std::shared_ptr<cell> goal) const noexcept {
    auto&& current = goal;
    auto breadcrumbs = std::make_shared<distances>(m_root);
    breadcrumbs->set(current, m_cells.at(current));

    while (current != m_root) {
        for (auto&& [neighbor, _] : current->get_links()) {
            if (neighbor && m_cells.at(neighbor) < m_cells.at(current)) {
                breadcrumbs->set(neighbor, m_cells.at(neighbor));
                current = neighbor;
                break;
            }
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