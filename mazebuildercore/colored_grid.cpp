#include <MazeBuilder/colored_grid.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_operations.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

using namespace mazes;

/// @brief
/// @param width 1
/// @param length 1
/// @param levels 1
colored_grid::colored_grid(unsigned int width, unsigned int length, unsigned int levels)
    : m_grid{ std::make_unique<grid>(width, length, levels) }, m_distances{ std::make_shared<distances>(0) }
{
}

std::string colored_grid::contents_of(const std::shared_ptr<cell>& c) const noexcept
{
    if (m_distances)
    {
        if (m_distances->contains(c->get_index()))
        {
            return std::to_string(m_distances->operator[](c->get_index()));
        }
    }

    // Fall back to default representation if no distance info available
    return m_grid->contents_of(c);
}

void colored_grid::initialize_distance_coloring(int start_index, int goal_index) noexcept
{
    m_distances = distances::path_to(m_grid.get(), start_index, goal_index);
}

std::uint32_t colored_grid::background_color_for(const std::shared_ptr<cell>& c) const noexcept
{
    using namespace std;

    if (!c)
    {
        return m_grid->background_color_for(cref(c));
    }

    // Only path cells are colored; all others use the grid default.
    if (!m_distances || !m_distances->contains(c->get_index()))
    {
        return m_grid->background_color_for(cref(c));
    }

    // 8-stop palette: deep blue (start) → red (goal)
    static constexpr std::array COLOR_PALETTE = {
        0x0015FFu, // deep blue
        0x0084FFu, // sky blue
        0x00E5FFu, // cyan
        0x00FF9Eu, // aqua green
        0x7DFF00u, // yellow-green
        0xFFF000u, // yellow
        0xFF9800u, // orange
        0xFF1A00u  // red
    };

    const int distance = (*m_distances)[c->get_index()];
    const int max_dist = m_distances->max().second;
    float normalized = max_dist > 0 ? static_cast<float>(distance) / static_cast<float>(max_dist) : 0.0f;
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    const auto bucket = static_cast<std::size_t>(lround(normalized * static_cast<float>(COLOR_PALETTE.size() - 1)));
    return COLOR_PALETTE[std::min(bucket, COLOR_PALETTE.size() - 1)];
}

// Delegate to embedded grid
grid_operations& colored_grid::operations() noexcept
{
    return m_grid->operations();
}

const grid_operations& colored_grid::operations() const noexcept
{
    return m_grid->operations();
}

void colored_grid::resize(const unsigned int rows, const unsigned int cols, const unsigned int levels) const noexcept
{
    m_grid->operations().resize(rows, cols, levels);
}
