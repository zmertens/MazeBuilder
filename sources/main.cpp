#include <random>
#include <memory>
#include <exception>
#include <iostream>
#include <string_view>
#include <sstream>
#include <future>
#include <algorithm>

#include "craft.h"
#include "grid.h"
#include "binary_tree.h"
#include "sidewinder.h"
#include "args_builder.h"
#include "maze_types_enum.h"
#include "writer.h"

int main(int argc, char* argv[]) {

#if defined(MAZE_DEBUG)
    static constexpr auto MAZE_BUILDER_VERSION = "maze_builder=[3.0.1] - DEBUG";
#else
    static constexpr auto MAZE_BUILDER_VERSION = "maze_builder=[3.0.1]";
#endif

    static constexpr auto HELP_MSG = R"help(
        Usages: maze_builder [OPTION]... [OUT_FILE]
        Generates mazes in ASCII-format or Wavefront object format
        Example: maze_builder -w 10 -l 10 -a binary_tree > out_maze.txt
        Options specify how to generate the maze and file output:
          -a, --algorithm    binary_tree [default], sidewinder
          -s, --seed         seed for the random number generator [mt19937]
          -w, --width        maze width [default=100]
          -y, --height       maze height [default=10]
          -l, --length       maze length [default=100]
          -i, --interactive  run program in interactive mode with a GUI
          -o, --output       stdout [default], plain text [.txt] or Wavefront object format [.obj]
          -h, --help         display this help message
          -v, --version      display program version
    )help";

    std::vector<std::string> args_vec;
    for (int i = 0; i < argc; i++) {
        args_vec.emplace_back(argv[i]);
    }

    // Force the -i param when compiling to the web
#if defined(__EMSCRIPTEN__)
    auto it_i = std::find(args_vec.begin(), args_vec.end(), "-i");
    auto it_interactive = std::find(args_vec.begin(), args_vec.end(), "--interactive");
    if (it_i == args_vec.end() && it_interactive == args_vec.end()) {
        args_vec.push_back("-i");
    }
#endif

    mazes::args_builder args (MAZE_BUILDER_VERSION, HELP_MSG, args_vec);
    auto&& args_map {args.build()};
    // this needs to get called after args.build() because of internal parsing
    auto state_of_args{ args.get_state() };
    
    if (state_of_args == mazes::args_state::JUST_NEEDS_HELP) {
        std::cout << HELP_MSG << std::endl;
        return EXIT_SUCCESS;
    } else if (state_of_args == mazes::args_state::JUST_NEEDS_VERSION) {
        std::cout << MAZE_BUILDER_VERSION << std::endl;
        return EXIT_SUCCESS;
    }

    auto use_this_for_seed = args_map.at("seed");
    auto get_int = [](int low, int high) -> int {
        using namespace std;
        random_device rd;
        seed_seq seed {rd()};
        mt19937 rng_engine {seed};
        uniform_int_distribution<int> dist {low, high};
        return dist(rng_engine);
    };

    auto get_maze_type_from_algo = [](const std::string& algo)->mazes::maze_types {
        using namespace std;
        if (algo.compare("binary_tree") == 0) {
            return mazes::maze_types::BINARY_TREE;
        } else if (algo.compare("sidewinder") == 0) {
            return mazes::maze_types::SIDEWINDER;
        } else {
#if defined(MAZE_DEBUG)
            cout << "INFO: Using default maze algorithm, \"binary_tree\"\n";
#endif
            return mazes::maze_types::BINARY_TREE;
        }
    };

    auto _grid {std::make_unique<mazes::grid>(args.get_width(), args.get_length(), args.get_height())};

    auto maze_factory = [&_grid, &get_int](mazes::maze_types maze_type) {
        switch (maze_type) {
            case mazes::maze_types::BINARY_TREE: {
                static mazes::binary_tree bt;
                // return std::async([&] {
                    return bt.run(std::ref(_grid), get_int);
                // });
            }
            case mazes::maze_types::SIDEWINDER: {
                static mazes::sidewinder sw;
                // return std::async([&] {
                    return sw.run(std::ref(_grid), get_int);
                // });
            }
        }
    };

    try {
        mazes::writer my_writer;
        auto write_func = [&my_writer, &args](auto data)->bool {
            return my_writer.write(args.get_output(), data);
        };
        std::packaged_task<bool(const std::string& data)> task_writes (write_func);

        mazes::maze_types my_maze_type = get_maze_type_from_algo(args.get_algorithm());
        bool success = false;
        if (args.is_interactive()) {
            // string views don't own the data, they have less copying overhead
            std::string_view window_title_view {"Maze Builder"};
            std::string_view version_view{ MAZE_BUILDER_VERSION };
            std::string_view help_view{ HELP_MSG };
            craft maze_builder_3D {window_title_view, version_view, help_view};
            success = maze_factory(my_maze_type);
            // this is redundant: first check grid is computed as maze, then run interactively
            // (different meanings of the word 'success' here)...
            if (success) {
                success = maze_builder_3D.run();
            }
        } else {
            success = maze_factory(my_maze_type);
        }

        if (success && !args.is_interactive()) {
            auto&& fut_writer = task_writes.get_future();
            std::stringstream ss;
            ss << *_grid.get();
            task_writes(ss.str());
            if (fut_writer.get()) {
#if defined(MAZE_DEBUG)
                std::cout << "Writing to file: " << args.get_output() << " complete!!" << std::endl;
#endif
            } else {
                std::cerr << "ERROR: Writing to file " << args.get_output() << std::endl;
            }
        } else if (!success){
            std::cerr << "ERROR: " << args.get_algorithm() << " failed!!" << std::endl;
        }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
}
