#ifndef BINARY_TREE_H
#define BINARY_TREE_H

#include <functional>
#include <memory>
#include <future>
#include <random>

#include "maze_algo_interface.h"

namespace mazes {

class grid_interface;
class cell;

class binary_tree : public maze_algo_interface {
public:
    bool run(std::unique_ptr<grid_interface> const& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept override;
private:
    bool run_on_cell(std::shared_ptr<cell> const& _cell, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept;
};
}
#endif // BINARY_TREE_H
