#ifndef CELL_H
#define CELL_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <mutex>

namespace mazes {

/// @file cell.h
/// @class cell
/// @brief Cell class for maze generation
class cell {

public:
    explicit cell(std::int32_t index = 0);

    void link(std::shared_ptr<cell> c1, std::shared_ptr<cell> c2, bool bidi=true);
    void unlink(std::shared_ptr<cell> c1, std::shared_ptr<cell> c2, bool bidi=true);

    const std::unordered_map<std::shared_ptr<cell>, bool>& get_links();
    bool is_linked(const std::shared_ptr<cell>& c);

    bool has_northern_neighbor() const noexcept;

    bool has_southern_neighbor() const noexcept;

    bool has_eastern_neighbor() const noexcept;

    bool has_western_neighbor() const noexcept;

    std::vector<std::shared_ptr<cell>> get_neighbors() const noexcept;

    int get_index() const;
	void set_index(int next_index) noexcept;

    std::shared_ptr<cell> get_north() const;
    std::shared_ptr<cell> get_south() const;
    std::shared_ptr<cell> get_east() const;
    std::shared_ptr<cell> get_west() const;
    
    void set_north(std::shared_ptr<cell> const& other);
    void set_south(std::shared_ptr<cell> const& other);
    void set_east(std::shared_ptr<cell> const& other);
    void set_west(std::shared_ptr<cell> const& other);
private:
    bool has_key(const std::shared_ptr<cell>& c);

    std::unordered_map<std::shared_ptr<cell>, bool> m_links;

    std::int32_t m_index;

    std::shared_ptr<cell> m_north;
    std::shared_ptr<cell> m_south;
    std::shared_ptr<cell> m_east;
    std::shared_ptr<cell> m_west;

    std::mutex m_mtx;
}; // class

} // namespace

#endif // CELL_H
