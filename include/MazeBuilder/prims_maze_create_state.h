#ifndef PRIMS_MAZE_CREATE_STATE_H
#define PRIMS_MAZE_CREATE_STATE_H

#include <MazeBuilder/algos.h>
#include <MazeBuilder/link_maze_and_create_state.h>

class randomizer;
class runtime_stack;

namespace mazes
{
    class prims_maze_create_state final : public link_maze_and_create_state
    {
    public:
        explicit prims_maze_create_state(const runtime_app::context& ctx, runtime_stack* stack);

        std::string_view create(const configurator& config, randomizer& rng) noexcept override;

    private:
        [[nodiscard]] algo get_algo_id() const noexcept override;

        std::string_view create_prims_maze(unsigned int rows, unsigned int cols, unsigned int levels,
            randomizer& rng) noexcept;
    };
} // namespace mazes

#endif // PRIMS_MAZE_CREATE_STATE_H
