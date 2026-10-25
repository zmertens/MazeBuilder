#include <MazeBuilder/cell_pool.h>

#include <MazeBuilder/cell.h>

using namespace mazes;

void cell_pool::acquire(std::uint8_t rows, std::uint8_t cols, std::uint8_t levels)
{
    release();

    const std::size_t total = static_cast<std::size_t>(rows) * cols * levels;
    if (m_cells.size() < total)
    {
        m_cells.reserve(total);
        while (m_cells.size() < total)
        {
            m_cells.push_back(std::make_shared<cell>());
        }
    }

    m_indices.reserve(total);
    const std::size_t per_level = static_cast<std::size_t>(rows) * cols;
    for (std::size_t i = 0; i < total; ++i)
    {
        const std::size_t in_level = per_level ? i % per_level : 0;
        m_cells[i]->set_area({static_cast<std::uint8_t>(in_level / cols),
                              static_cast<std::uint8_t>(in_level % cols)});
        m_indices.emplace(m_cells[i].get(), static_cast<int>(i));
    }
    m_active = total;
}

void cell_pool::release() noexcept
{
    for (std::size_t i = 0; i < m_active; ++i)
    {
        m_cells[i]->clear_links();
    }
    m_indices.clear();
    m_active = 0;
}

std::span<const std::shared_ptr<cell>> cell_pool::view() const noexcept
{
    return std::span<const std::shared_ptr<cell>>{m_cells}.first(m_active);
}

std::shared_ptr<cell> cell_pool::at(const int index) const noexcept
{
    if (index >= 0 && static_cast<std::size_t>(index) < m_active)
    {
        return m_cells[static_cast<std::size_t>(index)];
    }
    return nullptr;
}

int cell_pool::index_of(const cell *c) const noexcept
{
    const auto it = m_indices.find(c);
    return it == m_indices.end() ? -1 : it->second;
}
