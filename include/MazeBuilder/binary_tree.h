#ifndef BINARY_TREE_H
#define BINARY_TREE_H

#include <MazeBuilder/algo_interface.h>

#include <functional>
#include <memory>
#include <random>

namespace mazes {

class grid_interface;
class cell;

/// @brief Binary tree algorithm for generating mazes
class binary_tree : public algo_interface {
public:
    /// @brief Run the binary tree algorithm
    /// @param g
    /// @param get_int
    /// @param rng
    /// @return success or failure
    bool run(std::unique_ptr<grid_interface> const& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept override;
private:
    /// @brief Run the binary tree algorithm on a cell
    /// @param c
    /// @param get_int
    /// @param rng
    /// @return success or failure
    bool run_on_cell(std::shared_ptr<cell> const& c, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept;
};
}
#endif // BINARY_TREE_H
