#ifndef SIDEWINDER_HPP
#define SIDEWINDER_HPP

#include <functional>

#include "maze_algo_interface.h"

namespace mazes {

class grid;

class sidewinder : public maze_algo_interface {
public:

    bool run(grid& g, std::function<int(int, int)> const& get_int, bool interactive = false) noexcept override;
private:

};

}

#endif // SIDEWINDER_HPP