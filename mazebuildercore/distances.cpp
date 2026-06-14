#include <MazeBuilder/distances.h>

#include <algorithm>
#include <unordered_set>

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

std::shared_ptr<distances> distances::path_to(grid_interface *g, int32_t start_index, int32_t goal_index) noexcept
{
    auto path = std::make_shared<distances>(start_index);

    if (!g)
    {
        return path;
    }

    auto &ops = g->operations();
    const auto total_cells = static_cast<int32_t>(ops.num_cells());

    if (start_index < 0 || goal_index < 0 || start_index >= total_cells || goal_index >= total_cells)
    {
        return path;
    }

    if (start_index == goal_index)
    {
        return path;
    }

    std::unordered_map<int32_t, int32_t> parent;
    std::unordered_set<int32_t> visited;
    std::deque<int32_t> queue;

    queue.push_back(start_index);
    visited.insert(start_index);
    parent[start_index] = -1;

    bool found = false;

    while (!queue.empty())
    {
        const int32_t current_index = queue.front();
        queue.pop_front();

        if (current_index == goal_index)
        {
            found = true;
            break;
        }

        const auto current_cell = ops.search(current_index);
        if (!current_cell)
        {
            continue;
        }

        const auto neighbors = ops.get_neighbors(current_cell);
        for (const auto &neighbor : neighbors)
        {
            if (!neighbor || !current_cell->is_linked(neighbor))
            {
                continue;
            }

            const int32_t neighbor_index = neighbor->get_index();
            if (visited.find(neighbor_index) != visited.end())
            {
                continue;
            }

            visited.insert(neighbor_index);
            parent[neighbor_index] = current_index;
            queue.push_back(neighbor_index);
        }
    }

    if (!found)
    {
        return path;
    }

    std::vector<int32_t> path_indices;
    for (int32_t step = goal_index; step != -1; step = parent[step])
    {
        path_indices.push_back(step);
    }
    std::reverse(path_indices.begin(), path_indices.end());

    for (size_t i = 0; i < path_indices.size(); ++i)
    {
        path->set(path_indices[i], static_cast<int>(i));
    }

    return path;
}
