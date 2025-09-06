#ifndef CREATE_H
#define CREATE_H

#include <string>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/maze_interface.h>
#include <MazeBuilder/maze_factory.h>
#include <MazeBuilder/progress.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/stringify.h>

namespace mazes 
{

// Helper function to create maze and return str
static std::string create(const configurator &config)
{
    auto grid_creator = [](const configurator &config) -> std::unique_ptr<grid_interface>
    {
        return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
    };

    auto maze_creator = [grid_creator](const configurator &config) -> std::unique_ptr<maze_interface>
    {
        if (config.algo_id() != algo::DFS)
        {
            return nullptr;
        }

        grid_factory gf{};

        if (!gf.is_registered("g1"))
        {

            REQUIRE(gf.register_creator("g1", grid_creator));
        }

        if (auto igrid = gf.create("g1", cref(config)); igrid.has_value())
        {
            static dfs _dfs{};

            static randomizer rng{};

            rng.seed(config.seed());

            auto &&igridimpl = igrid.value();

            if (auto success = _dfs.run(igridimpl.get(), ref(rng)))
            {
                static stringify _stringifier;

                _stringifier.run(igridimpl.get(), ref(rng));

                return make_unique<maze_str>(igridimpl->operations().get_str());
            }
        }

        return nullptr;
    };

    std::string s{};

    auto duration = progress<>::duration([&config, maze_creator, &s]() -> bool
                                         {

        maze_factory mf{};

        if (!mf.is_registered("custom_maze")) {

            REQUIRE(mf.register_creator("custom_maze", maze_creator));
        }

        auto maze_optional = mf.create("custom_maze", cref(config));

        REQUIRE(maze_optional.has_value());

        s = maze_optional.value()->maze();
        
        return !s.empty(); });

    // pcout{} << duration.count() << " ms" << endl;

    return s;
} // create

} // namespace mazes

#endif // CREATE_H