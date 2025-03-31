#ifndef GRID_H
#define GRID_H

#include <functional>
#include <future>
#include <mutex>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

#include <MazeBuilder/grid_interface.h>

namespace mazes {

/// @file grid.h
/// @class grid
/// @brief General purpose grid class for maze generation
class grid : public grid_interface {
protected:
    void build_fut(std::vector<int> const& indices) noexcept;
public:
    /// @brief Friend classes
    friend class binary_tree;
    friend class dfs;
    friend class sidewinder;

    /// @brief Constructor for grid
    /// @param rows 
    /// @param columns 
    /// @param height 
    explicit grid(unsigned int rows = 1u, unsigned int columns = 1u, unsigned int height = 1u);

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

    /// @brief Initialize and configure
    /// @param callback notifies when configuration is complete
    /// @return 
    virtual bool get_future() noexcept;

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

    /// @brief
    /// @param index
    /// @return
    std::shared_ptr<cell> search(int index) const noexcept;

    /// @brief Get the count of cells in the grid
    /// @return The number of cells in the grid
    int count() const noexcept;
    
private:
    /// @brief Configure cells by neighbors (N, S, E, W)
    /// @param cells 
    void configure_cells(std::vector<std::shared_ptr<cell>>& cells) const noexcept;

    // Calculate the flat index from row and column
    std::function<int(unsigned int, unsigned int)> m_calc_index;

    std::tuple<unsigned int, unsigned int, unsigned int> m_dimensions;

    std::unordered_map<int, std::shared_ptr<cell>> m_cells;

    mutable std::promise<bool> m_config_promise;
    mutable std::once_flag m_config_flag;
    std::mutex m_config_mutex;
}; // class

} // namespace mazes

#endif // GRID_H
