#ifndef SIDEWINDER_HPP
#define SIDEWINDER_HPP

#include <functional>
#include <memory>
#include <future>

#include "maze_algo_interface.h"

namespace mazes {

class grid;
class sidewinder : public maze_algo_interface {
public:
    bool run(mazes::grid_ptr& _grid, std::function<int(int, int)> const& get_int, bool interactive = false) noexcept override;
private:
    mazes::grid_ptr m_grid;
};

}

#endif // SIDEWINDER_HPP
