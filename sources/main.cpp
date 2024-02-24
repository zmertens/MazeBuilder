#include "craft.h"
#include "args_handler.h"
#include "maze_builder_impl.h"

#include <exception>
#include <cstdio>
#include <iostream>

/*
maze_builder_impl maze_builder;
maze my_maze = maze_builder.seed(seed).interactive(i).build();
*/

int main(int argc, char* argv[]) {
    using namespace std;

    args_handler args (argc, argv);

    maze_builder_impl mazes (args);

    try {
        bool success = mazes.build(args.seed);
    } catch (std::exception& ex) {
        cerr << ex.what() << endl;    
    }

    return EXIT_SUCCESS;
}
