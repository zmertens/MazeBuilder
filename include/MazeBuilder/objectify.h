#ifndef OBJECTIFY_H
#define OBJECTIFY_H

#include <MazeBuilder/algo_interface.h>

namespace mazes {

class objectify : public algo_interface {
public:
    virtual bool run(std::unique_ptr<grid_interface> const& g, randomizer& rng) const noexcept override;
};
}

#endif // OBJECTIFY_H
