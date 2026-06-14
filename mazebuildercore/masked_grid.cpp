#include <MazeBuilder/masked_grid.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/mask.h>
#include <MazeBuilder/randomizer.h>

using namespace mazes;

masked_grid::masked_grid(mask m)
    : grid(m.rows(), m.columns()),
      m_mask(std::move(m))
{
}

std::shared_ptr<cell> masked_grid::search(int index) const noexcept
{
    const auto [rows, columns, levels] = get_dimensions();
    const int col = index % static_cast<int>(columns);
    const int row = (index / static_cast<int>(columns)) % static_cast<int>(rows);

    if (!m_mask(static_cast<unsigned int>(row), static_cast<unsigned int>(col)))
    {
        return nullptr;
    }

    return grid::search(index);
}

int masked_grid::num_cells() const noexcept
{
    return m_mask.count();
}

const mask &masked_grid::get_mask() const noexcept
{
    return m_mask;
}

std::shared_ptr<cell> masked_grid::random_cell(randomizer &rng) noexcept
{
    const auto [row, col] = m_mask.random_location(rng);
    const auto [rows, columns, levels] = get_dimensions();
    const int index = static_cast<int>(row * columns + col);
    return grid::search(index);
}
