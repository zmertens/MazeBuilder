#include <MazeBuilder/distances.h>

#include <MazeBuilder/grid_operations.h>

#include <algorithm>
#include <deque>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

using namespace mazes;

distances::distances(const std::int32_t root_index)
    : m_root_index(root_index)
{
    m_cells.insert_or_assign(root_index, 0);
}

int& distances::operator[](const std::int32_t index) noexcept
{
    return m_cells[index];
}

const int& distances::operator[](const std::int32_t index) const noexcept
{
    return m_cells.at(index);
}

void distances::set(const std::int32_t index, const int distance) noexcept
{
    m_cells[index] = distance;
}

bool distances::contains(const std::int32_t index) const noexcept
{
    return m_cells.contains(index);
}

std::pair<std::int32_t, int> distances::max() const noexcept
{
    int32_t max_index = m_root_index;
    int max_distance = 0;

    for (const auto& [index, distance] : m_cells)
    {
        if (distance > max_distance)
        {
            max_index = index;
            max_distance = distance;
        }
    }

    return {max_index, max_distance};
}

void distances::collect_keys(std::vector<std::int32_t>& indices) const noexcept
{
    indices.clear();
    for (const auto& index : m_cells | std::views::keys)
    {
        indices.push_back(index);
    }
}

std::shared_ptr<distances> distances::path_to(grid_interface* g, const std::int32_t start_index,
                                              const std::int32_t goal_index) noexcept
{
    auto path = std::make_shared<distances>(start_index);

    if (!g)
    {
        return path;
    }

    const auto& ops = g->operations();

    if (const auto total_cells = ops.num_cells(); start_index < 0 || goal_index < 0 || start_index >= total_cells ||
        goal_index >= total_cells)
    {
        return path;
    }

    if (start_index == goal_index)
    {
        return path;
    }

    std::unordered_map<std::int32_t, std::int32_t> parent;
    std::unordered_set<std::int32_t> visited;
    std::deque<std::int32_t> queue;

    queue.push_back(start_index);
    visited.insert(start_index);
    parent[start_index] = -1;

    bool found = false;

    while (!queue.empty())
    {
        const std::int32_t current_index = queue.front();
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

        for (const auto neighbors = ops.get_neighbors(current_cell); const auto& neighbor : neighbors)
        {
            if (!neighbor || !current_cell->is_linked(neighbor))
            {
                continue;
            }

            const std::int32_t neighbor_index = neighbor->get_index();
            if (visited.contains(neighbor_index))
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
    std::ranges::reverse(path_indices);

    for (size_t i = 0; i < path_indices.size(); ++i)
    {
        path->set(path_indices[i], static_cast<int>(i));
    }

    return path;
}
