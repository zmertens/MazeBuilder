#ifndef BINARY_TREE_H
#define BINARY_TREE_H

#include <functional>
#include <memory>
#include <future>

#include "maze_algo_interface.h"

namespace mazes {

class grid;
class cell;

class binary_tree : public maze_algo_interface {
public:
    bool run(std::unique_ptr<grid>& _grid, std::function<int(int, int)> const& get_int, bool interactive = false) noexcept override;
private:
    bool run_on_cell(std::shared_ptr<cell>& _cell, std::function<int(int, int)> const& get_int) noexcept;
};
}
#endif // BINARY_TREE_H
