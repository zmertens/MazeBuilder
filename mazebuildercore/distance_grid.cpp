#include <MazeBuilder/distance_grid.h>

#include <MazeBuilder/bytes.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_operations.h>

#include <algorithm>
#include <deque>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

using namespace mazes;

/// @brief Constructs a distance_grid object with specified dimensions and initializes the distance calculations.
/// @param width 1
/// @param length 1
/// @param levels 1
distance_grid::distance_grid(unsigned int width, unsigned int length, unsigned int levels)
    : m_grid{std::make_unique<grid>(width, length, levels)}
{
}

std::string distance_grid::contents_of(std::shared_ptr<cell> const& c) const noexcept
{
    if (m_distances && c)
    {
        // Check if the cell exists in our distance map
        if (m_distances->contains(c->get_index()))
        {
            if (const auto d = m_distances->operator[](c->get_index()); d >= 0)
            {
                return bytes::to_base36(d);
            }
        }
    }

    // Fall back to default representation if no distance info available
    return m_grid->contents_of(c);
}

std::uint32_t distance_grid::background_color_for(std::shared_ptr<cell> const& c) const noexcept
{
    return m_grid->background_color_for(cref(c));
}

/// @brief
/// @param start_index
/// @param end_index
void distance_grid::calculate_distances(const int start_index, const int end_index) noexcept
{
    try
    {
        const auto& grid_ops = m_grid->operations();

        const auto start_cell = grid_ops.search(start_index);
        if (!start_cell)
        {
            throw std::runtime_error("Invalid start cell index.");
        }

        // Create distances from start cell to all reachable cells
        m_distances = std::make_shared<distances>(start_cell->get_index());
        if (!m_distances)
        {
            throw std::runtime_error("Failed to create distances object.");
        }

        // Calculate distances from start cell to reachable cells using BFS.
        // Use contiguous visited storage to avoid hash lookups on the hot path.
        const int num_cells = grid_ops.num_cells();
        if (num_cells <= 0)
        {
            return;
        }

        std::vector<std::uint8_t> visited(static_cast<std::size_t>(num_cells), 0u);
        std::deque<std::int32_t> queue;

        const int max_distance = (end_index != -1) ? (end_index - start_index) : std::numeric_limits<int>::max();

        queue.push_back(start_index);
        if (start_index >= 0 && start_index < num_cells)
        {
            visited[static_cast<std::size_t>(start_index)] = 1u;
        }
        m_distances->set(start_index, 0);

        while (!queue.empty())
        {
            const std::int32_t current_index = queue.front();
            queue.pop_front();

            const auto current_cell = grid_ops.search(current_index);
            if (!current_cell)
            {
                continue;
            }

            const int current_distance = (*m_distances)[current_index];

            // If end_index is specified (not -1) and we've reached it, stop processing
            if (current_distance >= max_distance)
            {
                continue;
            }

            // Get all neighbors
            for (auto neighbors = grid_ops.get_neighbors(current_cell); const auto& neighbor : neighbors)
            {
                if (!neighbor)
                {
                    continue;
                }

                const std::int32_t neighbor_index = neighbor->get_index();

                // Skip if already visited
                if (neighbor_index < 0 || neighbor_index >= num_cells)
                {
                    continue;
                }

                if (visited[static_cast<std::size_t>(neighbor_index)] != 0u)
                {
                    continue;
                }

                // Only follow passages that exist (cells that are linked)
                if (!current_cell->is_linked(neighbor))
                {
                    continue;
                }

                const int next_distance = current_distance + 1;

                // If end_index is specified, don't exceed the distance range
                if (next_distance > max_distance)
                {
                    continue;
                }

                // Mark as visited and set distance
                visited[static_cast<std::size_t>(neighbor_index)] = 1u;
                m_distances->set(neighbor_index, next_distance);
                queue.push_back(neighbor_index);
            }
        }
    }
    catch (const std::exception&)
    {
    }
}

std::shared_ptr<distances> distance_grid::get_distances() const noexcept
{
    return this->m_distances;
}

// Delegate to embedded grid
grid_operations& distance_grid::operations() noexcept
{
    return m_grid->operations();
}

const grid_operations& distance_grid::operations() const noexcept
{
    return m_grid->operations();
}

void distance_grid::resize(const unsigned int rows, const unsigned int cols, const unsigned int levels) const noexcept
{
    m_grid->operations().resize(rows, cols, levels);
}
