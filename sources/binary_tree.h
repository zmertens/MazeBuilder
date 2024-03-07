#ifndef BINARY_TREE_H
#define BINARY_TREE_H

#include <functional>
#include <memory>
#include <future>

#include "maze_algo_interface.h"

namespace mazes {

class grid;

class binary_tree : public maze_algo_interface {
public:
    bool run(grid_ptr& _grid, std::function<int(int, int)> const& get_int, bool interactive = false) noexcept override;
private:
    
};
}
#endif // BINARY_TREE_H
