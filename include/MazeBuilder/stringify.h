#ifndef STRINGIFY_H
#define STRINGIFY_H

#include <MazeBuilder/algo_interface.h>

namespace mazes
{

    class stringify : public algo_interface
    {
    public:
        virtual bool run(grid_interface *g, randomizer &rng) const noexcept override;
    };
}

#endif // STRINGIFY_H
