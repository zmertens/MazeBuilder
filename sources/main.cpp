#include "craft.h"
#include "args_builder.h"
#include "maze_builder_impl.h"

#include <exception>
#include <cstdio>
#include <iostream>

/*
maze_builder_impl maze_builder;
maze my_maze = maze_builder.seed(seed).interactive(i).build();
maze my_maze2 = maze_builder.seed(seed).algo(algo_name).output(filename).build();
*/

int main(int argc, char* argv[]) {
    using namespace std;

    static constexpr auto MAZE_BUILDER_VERSION = "maze_builder=[2.0.1]";

    static const std::string HELP_MSG = R"help(
        Maze Builder Usages:
          1. ./maze_builder > default_maze.txt
          2. ./maze_builder --seed=1337 --algo=binary_tree -o bt.txt
          3. ./maze_builder -i -s 1337
    )help";

    try {
        mazes::args_builder args (MAZE_BUILDER_VERSION, HELP_MSG, argc, argv);
        mazes::args_state state_of_args {args.get_state()};

#if defined(DEBUGGING)
        std::cout << args << std::endl;
#endif

        if (state_of_args == mazes::args_state::JUST_NEEDS_HELP) {
            cout << HELP_MSG << endl;
            return EXIT_SUCCESS;
        } else if (state_of_args == mazes::args_state::JUST_NEEDS_VERSION) {
            cout << MAZE_BUILDER_VERSION << endl;
            return EXIT_SUCCESS;
        }

        if (args.is_interactive()) {
            maze_builder_impl maze_builder {"craft-sdl3"};
            auto my_maze = maze_builder.interactive(args.is_interactive()).seed(args.get_seed()).algo(args.get_algo()).output(args.get_output()).build();
            bool success = my_maze->run();        
        } else {
            maze_builder_impl bst_builder {"binary_tree_maze"};
            auto bst_maze = bst_builder.seed(args.get_seed()).build();
            bool success = bst_maze->run();
        }
    
    } catch (std::exception& ex) {
        cerr << "ERROR: " << ex.what() << endl; 
    }

    return EXIT_SUCCESS;
}
