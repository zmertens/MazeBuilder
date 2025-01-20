/// \file
/// The grid class implements a Binary Search Tree for performance and API reasons
/// Grid represents an ASCII-format maze in 2D, or Wavefront object file in 3D
///

#ifndef GRID_H
#define GRID_H

#include <vector>
#include <memory>
#include <sstream>
#include <string>
#include <functional>
#include <algorithm>
#include <optional>

#include "cell.h"
#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid_interface.h>

namespace mazes {

class grid : public grid_interface {
public:
    explicit grid(unsigned int rows, unsigned int columns, unsigned int height = 1u);

    virtual unsigned int get_rows() const noexcept override;
    virtual unsigned int get_columns() const noexcept override;
    virtual unsigned int get_height() const noexcept override;

    // Statistical queries
    unsigned int max_index(std::shared_ptr<cell> const& parent, unsigned int max = 0) const noexcept;
    unsigned int min_index(std::shared_ptr<cell> const& parent, unsigned int min = 0) const noexcept;

    virtual void populate_vec(std::vector<std::shared_ptr<cell>>& _cells) const noexcept override;
    virtual void preorder(std::vector<std::shared_ptr<cell>>& cells) const noexcept override;
    virtual void make_sorted_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept override;

    virtual void append(std::shared_ptr<grid_interface> const& other_grid) noexcept override;
    virtual void insert(std::shared_ptr<cell> const& parent, int index) noexcept override;
    virtual bool update(std::shared_ptr<cell>& parent, int old_index, int new_index) noexcept override;
    virtual std::shared_ptr<cell> search(std::shared_ptr<cell> const& start, int index) const noexcept override;
    virtual void del(std::shared_ptr<cell> parent, int index) noexcept override;

    virtual std::shared_ptr<cell> get_root() const noexcept override;

    virtual std::optional<std::string> contents_of(const std::shared_ptr<cell>& c) const noexcept override;
    virtual std::optional<std::uint32_t> background_color_for(const std::shared_ptr<cell>& c) const noexcept override;
private:
    bool create_binary_search_tree(const std::vector<int>& shuffled_indices);
    void configure_cells(std::vector<std::shared_ptr<cell>>& cells) noexcept;

    // Sort by youngest child cell -> oldest child
    void presort(std::shared_ptr<cell> const& parent, std::vector<std::shared_ptr<cell>>& cells) const noexcept;
    // Sort ascending per index-value
    void sort(std::shared_ptr<cell> const& parent, std::vector<std::shared_ptr<cell>>& cells_to_sort) const noexcept;
    void sort_by_row_then_col(std::vector<std::shared_ptr<cell>>& cells_to_sort) const noexcept;

    std::function<int(std::shared_ptr<cell> const&, std::shared_ptr<cell> const&)> m_sort_by_row_column;
    // Calculate the flat index from row and column
    std::function<int(unsigned int, unsigned int)> m_calc_index;
    std::shared_ptr<cell> m_binary_search_tree_root;
    const unsigned int m_rows, m_columns, m_height;
}; // class

} // namespace mazes

#endif // GRID_H
