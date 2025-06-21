#include <MazeBuilder/distances.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid_interface.h>

#include <deque>

#if defined(MAZE_DEBUG)
#include <iostream>
#endif

using namespace mazes;

distances::distances(int32_t root_index)
    : m_root_index(root_index) {
    m_cells.insert_or_assign(root_index, 0);
}

int& distances::operator[](int32_t index) noexcept {
    return m_cells[index];
}

const int& distances::operator[](int32_t index) const noexcept {
    return m_cells.at(index);
}

void distances::set(int32_t index, int distance) noexcept {
    m_cells[index] = distance;
}

bool distances::contains(int32_t index) const noexcept {
    return m_cells.find(index) != m_cells.end();
}

/// @brief Implementation of breadth-first search
/// @param goal_index 
/// @param g 
/// @return 
std::shared_ptr<distances> distances::path_to(std::unique_ptr<grid_interface> const& g, int32_t goal_index) const noexcept {
    // Create a new distances object to store the path
    auto path = std::make_shared<distances>(m_root_index);

    // If the goal index is the same as the root index, return an empty path
    if (goal_index == m_root_index) {
        return path;
    }

    path->set(m_root_index, 0);

    // Parent map to reconstruct the path
    std::unordered_map<int32_t, int32_t> parent;

    // BFS queue
    std::deque<int32_t> q;
    q.push_back(goal_index);
    parent[goal_index] = -1;

    static constexpr auto MAX_ITERATIONS = 1000000;
    auto current_iteration = 0;

    while (!q.empty()) {
#if defined(MAZE_DEBUG)
        if (current_iteration++ > MAX_ITERATIONS) {
            std::cerr << "Error: BFS exceeded maximum iterations." << std::endl;
            return path;
        }
#endif

        int32_t current_index = q.front();
        q.pop_front();

        // Retrieve the current cell
//        auto current_cell = g.search(current_index);
//        if (!current_cell) {
//#if defined(MAZE_DEBUG)
//            std::cerr << "Error: grid::search returned nullptr for index " << current_index << std::endl;
//#endif
//            continue;
//        }

        // Process each neighbor that has a passage (linked cells)
        //auto neighbors = g.get_neighbors(current_cell);
        //for (const auto& neighbor : neighbors) {
        //    if (!neighbor) continue;

        //    // Only follow passages that exist (cells that are linked)
        //    if (!current_cell->is_linked(neighbor)) {
        //        continue;
        //    }

        //    int32_t neighbor_index = neighbor->get_index();

        //    // Skip if already visited
        //    if (parent.find(neighbor_index) != parent.cend()) {
        //        continue;
        //    }

        //    // Mark the parent
        //    parent[neighbor_index] = current_index;
        //    q.push_back(neighbor_index);

        //    // If we reach the root index, reconstruct the path
        //    if (neighbor_index == m_root_index) {
        //        int32_t step = m_root_index;
        //        int distance = 0;

        //        // Build the path
        //        while (step != -1) {
        //            path->set(step, distance);
        //            step = parent[step];
        //            distance += 1;
        //        }

        //        return path;
        //    }
        //}
    }

    return std::make_shared<distances>(m_root_index);
}

std::pair<int32_t, int> distances::max() const noexcept {
    int32_t max_index = m_root_index;
    int max_distance = 0;

    for (const auto& [index, distance] : m_cells) {
        if (distance > max_distance) {
            max_index = index;
            max_distance = distance;
        }
    }

    return { max_index, max_distance };
}

void distances::collect_keys(std::vector<int32_t>& indices) const noexcept {
    indices.clear();
    for (const auto& [index, _] : m_cells) {
        indices.push_back(index);
    }
}
