#include <MazeBuilder/distances.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>

#include <deque>

#if defined(MAZE_DEBUG)
#include <iostream>
#endif

using namespace mazes;

distances::distances(int32_t root_index)
    : m_root_index(root_index)
{
    m_cells.insert_or_assign(root_index, 0);
}

int &distances::operator[](int32_t index) noexcept
{
    return m_cells[index];
}

const int &distances::operator[](int32_t index) const noexcept
{
    return m_cells.at(index);
}

void distances::set(int32_t index, int distance) noexcept
{
    m_cells[index] = distance;
}

bool distances::contains(int32_t index) const noexcept
{
    return m_cells.find(index) != m_cells.end();
}

/// @brief Implementation of breadth-first search
/// @param goal_index
/// @param g
/// @return
std::shared_ptr<distances> distances::path_to(std::unique_ptr<grid_interface> const &g, int32_t goal_index) const noexcept
{
    // Create a new distances object to store the path
    auto path = std::make_shared<distances>(m_root_index);

    // If the goal index is the same as the root index, return path with just the root
    if (goal_index == m_root_index)
    {
        return path;
    }

    // Parent map to reconstruct the path
    std::unordered_map<int32_t, int32_t> parent;
    std::unordered_map<int32_t, bool> visited;

    // BFS queue
    std::deque<int32_t> q;
    q.push_back(m_root_index);
    visited[m_root_index] = true;
    parent[m_root_index] = -1;

    static constexpr auto MAX_ITERATIONS = 1000000;
    auto current_iteration = 0;

    try
    {
        // Get the grid operations interface
        auto &ops = g->operations();

        while (!q.empty())
        {
#if defined(MAZE_DEBUG)
            if (current_iteration++ > MAX_ITERATIONS)
            {
                std::cerr << "Error: BFS exceeded maximum iterations." << std::endl;
                return path;
            }
#endif

            int32_t current_index = q.front();
            q.pop_front();

            // If we reached the goal, reconstruct the path
            if (current_index == goal_index)
            {
                // Reconstruct path from goal to root
                std::vector<int32_t> path_indices;
                int32_t step = goal_index;

                while (step != -1)
                {
                    path_indices.push_back(step);
                    step = parent[step];
                }

                // Set distances in the path (distance 0 for root, increasing towards goal)
                for (size_t i = 0; i < path_indices.size(); ++i)
                {
                    int distance = static_cast<int>(path_indices.size() - 1 - i);
                    path->set(path_indices[i], distance);
                }

                return path;
            }

            // Retrieve the current cell
            auto current_cell = ops.search(current_index);
            if (!current_cell)
            {
#if defined(MAZE_DEBUG)
                std::cerr << "Error: grid::search returned nullptr for index " << current_index << std::endl;
#endif
                continue;
            }

            // Process each neighbor that has a passage (linked cells)
            auto neighbors = ops.get_neighbors(current_cell);
            for (const auto &neighbor : neighbors)
            {
                if (!neighbor)
                    continue;

                int32_t neighbor_index = neighbor->get_index();

                // Skip if already visited
                if (visited.find(neighbor_index) != visited.end())
                {
                    continue;
                }

                // Only follow passages that exist (cells that are linked)
                if (!current_cell->is_linked(neighbor))
                {
                    continue;
                }

                // Mark as visited and add to queue
                visited[neighbor_index] = true;
                parent[neighbor_index] = current_index;
                q.push_back(neighbor_index);
            }
        }
    }
    catch (const std::exception &e)
    {
#if defined(MAZE_DEBUG)
        std::cerr << "Exception in path_to: " << e.what() << std::endl;
#endif
    }

    // No path found, return empty path
    return std::make_shared<distances>(m_root_index);
}

std::pair<int32_t, int> distances::max() const noexcept
{
    int32_t max_index = m_root_index;
    int max_distance = 0;

    for (const auto &[index, distance] : m_cells)
    {
        if (distance > max_distance)
        {
            max_index = index;
            max_distance = distance;
        }
    }

    return {max_index, max_distance};
}

void distances::collect_keys(std::vector<int32_t> &indices) const noexcept
{
    indices.clear();
    for (const auto &[index, _] : m_cells)
    {
        indices.push_back(index);
    }
}
