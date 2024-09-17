#ifndef DFS_H
#define DFS_H

#include <vector>

#include "maze_algo_interface.h"

namespace mazes {
    class dfs : public maze_algo_interface {
    public:
    virtual bool run(const std::unique_ptr<grid_interface>& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept override;
    };
}

#endif // DFS_H