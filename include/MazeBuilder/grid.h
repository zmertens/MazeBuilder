#ifndef GRID_H
#define GRID_H

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <utility>
#include <optional>

#include <MazeBuilder/grid_interface.h>

namespace mazes {

/// @brief General purpose grid class for maze generation
class grid : public grid_interface {

public:
    /// @brief Friend classes
    friend class binary_tree;
    friend class dfs;
    friend class sidewinder;

    /// @brief Constructor for grid
    /// @param rows 
    /// @param columns 
    /// @param height 
    explicit grid(unsigned int rows, unsigned int columns, unsigned int height = 1u);

    /// @brief Constructor for grid
    /// @param dimensions 
    explicit grid(std::tuple<unsigned int, unsigned int, unsigned int> dimensions);

    /// @brief Copy constructor
    /// @param other 
    grid(const grid& other);
    
    /// @brief Assignment operator
    /// @param other 
    /// @return 
    grid& operator=(const grid& other);
    
    /// @brief Move constructor
    /// @param other 
    grid(grid&& other) noexcept;
    
    /// @brief Move assignment operator
    /// @param other 
    /// @return 
    grid& operator=(grid&& other) noexcept;
    
    /// @brief Destructor
    virtual ~grid();

    // Statics
    template <typename G = std::unique_ptr<grid_interface>>
    static std::optional<std::unique_ptr<grid>> make_opt(const G g) noexcept {
        if (const auto* gg = dynamic_cast<const grid*>(g.get())) {
            return std::make_optional(std::make_unique<grid>(*gg));
        }
        return std::nullopt;
    }

    // Overrides

    /// @brief Provides dimensions of grid in no assumed ordering
    /// @return 
    virtual std::tuple<unsigned int, unsigned int, unsigned int> get_dimensions() const noexcept override;

    /// @brief Convert a 2D grid to a vector of cells (sorted by row then column)
    /// @param cells 
    virtual void to_vec(std::vector<std::shared_ptr<cell>>& cells) const noexcept override;

    /// @brief Convert a 2D grid to a vector of vectors of cells (sorted by row then column)
    /// @param cells
    virtual void to_vec2(std::vector<std::vector<std::shared_ptr<cell>>>& cells) const noexcept override;

    /// @brief Get detailed information of a cell in the grid
    /// @param c 
    /// @return 
    virtual std::optional<std::string> contents_of(const std::shared_ptr<cell>& c) const noexcept override;

    /// @brief Get the background color for a cell in the grid
    /// @param c 
    /// @return 
    virtual std::optional<std::uint32_t> background_color_for(const std::shared_ptr<cell>& c) const noexcept override;

    // CRUD operations

    /// @brief 
    /// @param other_grid 
    virtual void append(std::shared_ptr<grid_interface> const& other_grid) noexcept;

    /// @brief 
    /// @param parent 
    /// @param index 
    virtual void insert(std::shared_ptr<cell> const& parent, int index) noexcept;

    /// @brief 
    /// @param parent 
    /// @param old_index 
    /// @param new_index 
    /// @return 
    virtual bool update(std::shared_ptr<cell>& parent, int old_index, int new_index) noexcept;

    /// @brief
    /// @param start
    /// @param index
    /// @return
    virtual std::shared_ptr<cell> search(std::shared_ptr<cell> const& start, int index) const noexcept;

    /// @brief
    /// @param parent
    /// @param index
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
