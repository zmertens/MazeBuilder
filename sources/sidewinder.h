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
    bool run(std::unique_ptr<grid>& _grid, std::function<int(int, int)> const& get_int, bool interactive = false) noexcept override;

};

}

#endif // SIDEWINDER_HPP
