/// \file
/// The grid class implements a Binary Search Tree for performance reasons
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

#include <MazeBuilder/grid_interface.h>

namespace mazes {

    class grid : public grid_interface {

    public:

    friend class binary_tree;
    friend class dfs;
    friend class sidewinder;

    explicit grid(unsigned int rows, unsigned int columns, unsigned int height = 1u);

    // Overrides

    /// @brief Provides dimensions of grid in no assumed ordering
    /// @return 
    virtual std::tuple<unsigned int, unsigned int, unsigned int> get_dimensions() const noexcept override;

    virtual void to_vec(std::vector<std::shared_ptr<cell>>& _cells) const noexcept override;
    virtual void to_vec2(std::vector<std::vector<std::shared_ptr<cell>>>& cells) const noexcept override;

    virtual std::optional<std::string> contents_of(const std::shared_ptr<cell>& c) const noexcept override;
    virtual std::optional<std::uint32_t> background_color_for(const std::shared_ptr<cell>& c) const noexcept override;

    // CRUD operations

    virtual void append(std::shared_ptr<grid_interface> const& other_grid) noexcept;
    virtual void insert(std::shared_ptr<cell> const& parent, int index) noexcept;
    virtual bool update(std::shared_ptr<cell>& parent, int old_index, int new_index) noexcept;
    virtual std::shared_ptr<cell> search(std::shared_ptr<cell> const& start, int index) const noexcept;
    virtual void del(std::shared_ptr<cell> parent, int index) noexcept;

protected:
    std::shared_ptr<cell> m_binary_search_tree_root;
    
private:
    bool create_binary_search_tree(const std::vector<int>& shuffled_indices);
    void configure_cells(std::vector<std::shared_ptr<cell>>& cells) noexcept;

    // Sort by youngest child cell -> oldest child
    void presort(std::shared_ptr<cell> const& parent, std::vector<std::shared_ptr<cell>>& cells) const noexcept;

    // Sort ascending per index-value
    void inorder(std::shared_ptr<cell> const& parent, std::vector<std::shared_ptr<cell>>& cells) const noexcept;

    // Sort ascending per index-value
    void sort_by_row_then_col(std::vector<std::shared_ptr<cell>>& cells_to_sort) const noexcept;

    std::function<int(std::shared_ptr<cell> const&, std::shared_ptr<cell> const&)> m_sort_by_row_column;
    // Calculate the flat index from row and column
    std::function<int(unsigned int, unsigned int)> m_calc_index;

    std::tuple<unsigned int, unsigned int, unsigned int> m_dimensions;
}; // class

} // namespace mazes

#endif // GRID_H
