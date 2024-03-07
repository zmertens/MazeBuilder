#ifndef CELL_H
#define CELL_H

#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>

namespace mazes {

class cell {
public:
    cell(unsigned int row, unsigned int column);
    void link(std::shared_ptr<cell> c1, std::shared_ptr<cell> c2, bool bidi=true);
    void unlink(std::shared_ptr<cell> c1, std::shared_ptr<cell> c2, bool bidi=true);
    std::unordered_map<std::shared_ptr<cell>, bool> get_links() const;
    bool is_linked(const std::shared_ptr<cell>& c) const;

    std::vector<std::shared_ptr<cell>> get_neighbors() const noexcept;

    unsigned int get_row() const;
    unsigned int get_column() const;

    std::shared_ptr<cell> get_north() const;
    std::shared_ptr<cell> get_south() const;
    std::shared_ptr<cell> get_east() const;
    std::shared_ptr<cell> get_west() const;
    
    void set_north(std::shared_ptr<cell> const& other);
    void set_south(std::shared_ptr<cell> const& other);
    void set_east(std::shared_ptr<cell> const& other);
    void set_west(std::shared_ptr<cell> const& other);

    bool operator==(std::shared_ptr<cell> const& other);

    std::shared_ptr<cell> get_cell() const;
private:
    bool has_key(const std::shared_ptr<cell>& c) const;

    std::unordered_map<std::shared_ptr<cell>, bool> m_links;

    const unsigned int m_row, m_column;

    std::shared_ptr<cell> m_north;
    std::shared_ptr<cell> m_south;
    std::shared_ptr<cell> m_east;
    std::shared_ptr<cell> m_west;
};

using shared_cell_ptr = std::shared_ptr<cell>;

} // namespace

#endif // CELL_H
