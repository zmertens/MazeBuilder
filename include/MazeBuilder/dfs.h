#ifndef DFS_H
#define DFS_H

#include <vector>

#include <MazeBuilder/algos_interface.h>

namespace mazes {

class grid_interface;

    class dfs : public algos_interface {
    public:
    virtual bool run(const std::unique_ptr<grid_interface>& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept override;
    };
}

#endif // DFS_H