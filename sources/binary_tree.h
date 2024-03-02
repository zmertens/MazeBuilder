#ifndef BINARY_TREE_H
#define BINARY_TREE_H

#include <functional>

#include "maze_algo_interface.h"

namespace mazes {

class grid;

class binary_tree : public maze_algo_interface {
public:

    bool run(grid& g, std::function<int(int, int)> const& get_int) const noexcept override;
private:

};
}
#endif // BINARY_TREE_H