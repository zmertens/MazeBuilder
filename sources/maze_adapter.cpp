#include <MazeBuilder/maze_adapter.h>
#include <MazeBuilder/cell.h>

#include <algorithm>
#include <stdexcept>
#include <string>

using namespace mazes;

maze_adapter::maze_adapter(const container_type& cells) noexcept
    : m_cells(cells) {
}

maze_adapter::maze_adapter(container_type&& cells) noexcept
    : m_cells(std::move(cells)) {
}

// Element access
maze_adapter::const_reference maze_adapter::at(size_type index) const {
    if (index >= m_cells.size()) {
        throw std::out_of_range("maze_adapter::at: index " + std::to_string(index) +
            " is out of range (size: " + std::to_string(m_cells.size()) + ")");
    }
    return m_cells[index];
}

maze_adapter::const_reference maze_adapter::operator[](size_type index) const noexcept {
    return m_cells[index];
}

maze_adapter::const_reference maze_adapter::front() const {
    if (m_cells.empty()) {
        throw std::runtime_error("maze_adapter::front: container is empty");
    }
    return m_cells.front();
}

maze_adapter::const_reference maze_adapter::back() const {
    if (m_cells.empty()) {
        throw std::runtime_error("maze_adapter::back: container is empty");
    }
    return m_cells.back();
}

maze_adapter::const_pointer maze_adapter::data() const noexcept {
    return m_cells.data();
}

// Iterators
maze_adapter::const_iterator maze_adapter::begin() const noexcept {
    return m_cells.begin();
}

maze_adapter::const_iterator maze_adapter::cbegin() const noexcept {
    return m_cells.cbegin();
}

maze_adapter::const_iterator maze_adapter::cbegin(size_type start_index, size_type count) const {
    if (start_index >= m_cells.size()) {
        throw std::out_of_range("maze_adapter::cbegin: start_index " + std::to_string(start_index) +
            " is out of range (size: " + std::to_string(m_cells.size()) + ")");
    }

    if (start_index + count > m_cells.size()) {
        throw std::out_of_range("maze_adapter::cbegin: range [" + std::to_string(start_index) +
            ", " + std::to_string(start_index + count) + ") exceeds container size: " +
            std::to_string(m_cells.size()));
    }

    return m_cells.cbegin() + start_index;
}

maze_adapter::const_iterator maze_adapter::end() const noexcept {
    return m_cells.end();
}

maze_adapter::const_iterator maze_adapter::cend() const noexcept {
    return m_cells.cend();
}

maze_adapter::const_reverse_iterator maze_adapter::rbegin() const noexcept {
    return m_cells.rbegin();
}

maze_adapter::const_reverse_iterator maze_adapter::crbegin() const noexcept {
    return m_cells.crbegin();
}

maze_adapter::const_reverse_iterator maze_adapter::rend() const noexcept {
    return m_cells.rend();
}

maze_adapter::const_reverse_iterator maze_adapter::crend() const noexcept {
    return m_cells.crend();
}

// Capacity
bool maze_adapter::empty() const noexcept {
    return m_cells.empty();
}

maze_adapter::size_type maze_adapter::size() const noexcept {
    return m_cells.size();
}

maze_adapter::size_type maze_adapter::max_size() const noexcept {
    return m_cells.max_size();
}

// Search operations
maze_adapter::const_iterator maze_adapter::find(int32_t index) const noexcept {
    return std::find_if(m_cells.begin(), m_cells.end(),
        [index](const value_type& cell) {
            return cell && cell->get_index() == index;
        });
}

maze_adapter::size_type maze_adapter::count(int32_t index) const noexcept {
    return std::count_if(m_cells.begin(), m_cells.end(),
        [index](const value_type& cell) {
            return cell && cell->get_index() == index;
        });
}

bool maze_adapter::contains(int32_t index) const noexcept {
    return find(index) != m_cells.end();
}

// Subview operations
maze_adapter maze_adapter::substr(size_type pos) const {
    if (pos > m_cells.size()) {
        throw std::out_of_range("maze_adapter::substr: pos " + std::to_string(pos) +
            " is out of range (size: " + std::to_string(m_cells.size()) + ")");
    }

    return maze_adapter(m_cells.begin() + pos, m_cells.end());
}

maze_adapter maze_adapter::substr(size_type pos, size_type len) const {
    if (pos > m_cells.size()) {
        throw std::out_of_range("maze_adapter::substr: pos " + std::to_string(pos) +
            " is out of range (size: " + std::to_string(m_cells.size()) + ")");
    }

    size_type actual_len = std::min(len, m_cells.size() - pos);

    return maze_adapter(m_cells.begin() + pos, m_cells.begin() + pos + actual_len);
}

// Utility operations
maze_adapter maze_adapter::remove_nulls() const {
    container_type filtered;
    std::copy_if(m_cells.begin(), m_cells.end(), std::back_inserter(filtered),
        [](const value_type& cell) {
            return cell != nullptr;
        });

    return maze_adapter(std::move(filtered));
}

maze_adapter maze_adapter::sort_by_index() const {
    container_type sorted = m_cells;
    std::sort(sorted.begin(), sorted.end(),
        [](const value_type& a, const value_type& b) {
            if (!a && !b) return false;
            if (!a) return true;  // nulls first
            if (!b) return false;
            return a->get_index() < b->get_index();
        });

    return maze_adapter(std::move(sorted));
}

std::vector<int32_t> maze_adapter::get_indices() const {
    std::vector<int32_t> indices;
    indices.reserve(m_cells.size());

    for (const auto& cell : m_cells) {
        if (cell) {
            indices.push_back(cell->get_index());
        }
    }

    return indices;
}

// Comparison operators
bool maze_adapter::operator==(const maze_adapter& other) const noexcept {
    if (m_cells.size() != other.m_cells.size()) {
        return false;
    }

    return std::equal(m_cells.begin(), m_cells.end(), other.m_cells.begin(),
        [](const value_type& a, const value_type& b) {
            if (!a && !b) return true;
            if (!a || !b) return false;
            return a->get_index() == b->get_index();
        });
}

bool maze_adapter::operator!=(const maze_adapter& other) const noexcept {
    return !(*this == other);
}

// Private methods
void maze_adapter::validate_range(size_type pos, size_type len) const {
    if (pos > m_cells.size()) {
        throw std::out_of_range("maze_adapter: pos " + std::to_string(pos) +
            " is out of range (size: " + std::to_string(m_cells.size()) + ")");
    }

    if (len > 0 && pos + len > m_cells.size()) {
        throw std::out_of_range("maze_adapter: range [" + std::to_string(pos) +
            ", " + std::to_string(pos + len) + ") exceeds container size: " +
            std::to_string(m_cells.size()));
    }
}
