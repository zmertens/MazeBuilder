#include <MazeBuilder/grid_range.h>
#include <MazeBuilder/cell.h>

#include <algorithm>
#include <stdexcept>
#include <utility>

using namespace mazes;

// grid_iterator implementation
grid_iterator::grid_iterator(
    std::unordered_map<int, std::shared_ptr<cell>>& cells,
    std::tuple<unsigned int, unsigned int, unsigned int> dimensions,
    int current_index,
    int end_index,
    std::function<std::shared_ptr<cell>(int)> create_cells_func)
    : m_cells(cells)
    , m_dimensions(dimensions)
    , m_current_index(current_index)
    , m_end_index(end_index)
    , m_create_cells_func(create_cells_func)
{
}

grid_iterator::value_type grid_iterator::operator*() const
{
    if (m_current_index >= m_end_index || !is_valid_index(m_current_index))
    {
        return nullptr;
    }
    return get_or_create_cell(m_current_index);
}

grid_iterator& grid_iterator::operator++()
{
    ++m_current_index;
    return *this;
}

grid_iterator grid_iterator::operator++(int)
{
    grid_iterator tmp = *this;
    ++(*this);
    return tmp;
}

bool grid_iterator::operator==(const grid_iterator& other) const
{
    return m_current_index == other.m_current_index;
}

bool grid_iterator::operator!=(const grid_iterator& other) const
{
    return !(*this == other);
}

bool grid_iterator::is_valid_index(int index) const noexcept
{
    auto [rows, columns, levels] = m_dimensions;
    int max_index = static_cast<int>(rows * columns * levels);
    return index >= 0 && index < max_index;
}

std::shared_ptr<cell> grid_iterator::get_or_create_cell(int index) const
{
    // First check if cell already exists
    auto cell_it = m_cells.find(index);
    if (cell_it != m_cells.end())
    {
        return cell_it->second;
    }

    // Use the provided creation function if available
    if (m_create_cells_func && is_valid_index(index))
    {
        return m_create_cells_func(index);
    }

    // Fallback: create cell directly if within bounds
    if (is_valid_index(index))
    {
        auto new_cell = std::make_shared<cell>(index);
        m_cells[index] = new_cell;
        return new_cell;
    }

    return nullptr;
}

// grid_range implementation
grid_range::grid_range(
    std::unordered_map<int, std::shared_ptr<cell>>& cells,
    std::tuple<unsigned int, unsigned int, unsigned int> dimensions,
    std::function<std::shared_ptr<cell>(int)> create_cells_func)
    : m_cells(cells)
    , m_dimensions(dimensions)
    , m_start_index(0)
    , m_end_index(max_valid_index())
    , m_create_cells_func(create_cells_func)
{
}

grid_range::grid_range(
    std::unordered_map<int, std::shared_ptr<cell>>& cells,
    std::tuple<unsigned int, unsigned int, unsigned int> dimensions,
    int start_index,
    int end_index,
    std::function<std::shared_ptr<cell>(int)> create_cells_func)
    : m_cells(cells)
    , m_dimensions(dimensions)
    , m_create_cells_func(create_cells_func)
{
    int max_index = max_valid_index();
    
    // Clamp start_index to valid range
    m_start_index = std::max(0, std::min(start_index, max_index));
    
    // Clamp end_index to valid range, but ensure it's not less than start_index
    m_end_index = std::max(m_start_index, std::min(end_index, max_index));
}

grid_iterator grid_range::begin()
{
    return grid_iterator(m_cells, m_dimensions, m_start_index, m_end_index, m_create_cells_func);
}

grid_iterator grid_range::end()
{
    return grid_iterator(m_cells, m_dimensions, m_end_index, m_end_index, m_create_cells_func);
}

grid_iterator grid_range::begin() const
{
    return grid_iterator(const_cast<std::unordered_map<int, std::shared_ptr<cell>>&>(m_cells), 
                        m_dimensions, m_start_index, m_end_index, m_create_cells_func);
}

grid_iterator grid_range::end() const
{
    return grid_iterator(const_cast<std::unordered_map<int, std::shared_ptr<cell>>&>(m_cells), 
                        m_dimensions, m_end_index, m_end_index, m_create_cells_func);
}

size_t grid_range::size() const noexcept
{
    return static_cast<size_t>(std::max(0, m_end_index - m_start_index));
}

bool grid_range::empty() const noexcept
{
    return m_start_index >= m_end_index;
}

std::vector<std::shared_ptr<cell>> grid_range::to_vector() const
{
    std::vector<std::shared_ptr<cell>> result;
    
    // For the full grid range, return all existing cells sorted by index
    if (m_start_index == 0 && m_end_index == max_valid_index())
    {
        result.reserve(m_cells.size());
        
        // Create sorted vector of all existing cells
        std::vector<std::pair<int, std::shared_ptr<cell>>> indexed_cells;
        indexed_cells.reserve(m_cells.size());
        
        for (const auto& [index, cell_ptr] : m_cells)
        {
            if (cell_ptr)
            {
                indexed_cells.emplace_back(index, cell_ptr);
            }
        }
        
        std::sort(indexed_cells.begin(), indexed_cells.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        
        for (const auto& [index, cell_ptr] : indexed_cells)
        {
            result.push_back(cell_ptr);
        }
    }
    else
    {
        // For subset ranges, iterate through the range
        result.reserve(size());
        
        for (auto it = begin(); it != end(); ++it)
        {
            if (auto cell_ptr = *it)
            {
                result.push_back(cell_ptr);
            }
        }
    }
    
    return result;
}

void grid_range::clear()
{
    // Remove cells within this range
    for (int i = m_start_index; i < m_end_index; ++i)
    {
        auto it = m_cells.find(i);
        if (it != m_cells.end())
        {
            // Clear cell links before removing
            if (it->second)
            {
                it->second->cleanup_links();
            }
            m_cells.erase(it);
        }
    }
}

bool grid_range::set_from_vector(const std::vector<std::shared_ptr<cell>>& cells)
{
    // For full grid range, clear all existing cells first
    if (m_start_index == 0 && m_end_index == max_valid_index())
    {
        m_cells.clear();
    }
    else
    {
        // For subset ranges, only clear cells within the range
        clear();
    }
    
    // Add all provided cells
    for (const auto& cell_ptr : cells)
    {
        if (cell_ptr)
        {
            int index = cell_ptr->get_index();
            // For full range, accept all cells; for subset, only cells in range
            if (m_start_index == 0 && m_end_index == max_valid_index())
            {
                m_cells[index] = cell_ptr;
            }
            else if (index >= m_start_index && index < m_end_index)
            {
                m_cells[index] = cell_ptr;
            }
        }
    }
    
    return true;
}

int grid_range::max_valid_index() const noexcept
{
    auto [rows, columns, levels] = m_dimensions;
    return static_cast<int>(rows * columns * levels);
}
