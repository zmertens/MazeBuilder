#ifndef GRID_H
#define GRID_H

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/observable_interface.h>

namespace mazes {

/// @file grid.h
/// @class grid
/// @brief General purpose grid class for maze generation
class grid : public grid_interface, public observable_interface {
public:

    /// @brief 
    /// @param indices 
    /// @return 
    virtual void start_configuration(const std::vector<int>& indices) noexcept;
public:

    /// @brief Friend classes
    friend class binary_tree;
    friend class dfs;
    friend class sidewinder;

    /// @brief 
    /// @param r 
    /// @param c 
    /// @param l 
    grid(unsigned int r = 1u, unsigned int c = 1u, unsigned int l = 1u);

    /// @brief 
    /// @param dimens 
    grid(std::tuple<unsigned int, unsigned int, unsigned int> dimens);

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

    // Overrides

    /// @brief 
    /// @param observer 
    void register_observer(std::function<bool(void)> const& observer) noexcept override;

    /// @brief 
    /// @return 
    bool is_observed() noexcept override;

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
    int num_cells() const noexcept;

protected:
    /// @brief 
    /// @param result 
    void notify_observers() noexcept;

private:
    /// @brief Configure cells by neighbors (N, S, E, W)
    /// @param cells 
    void configure_cells(std::vector<std::shared_ptr<cell>>& cells) const noexcept;

    /// @brief Calculate the flat index for a 2D grid
    std::function<int(unsigned int, unsigned int)> m_calc_index;

    std::unordered_map<int, std::shared_ptr<cell>> m_cells;
    std::tuple<unsigned int, unsigned int, unsigned int> m_dimensions;

    std::vector<std::function<bool(void)>> m_observers;
    std::mutex m_observers_mutex;

    std::atomic<bool> m_configured;
}; // class

} // namespace mazes

#endif // GRID_H
