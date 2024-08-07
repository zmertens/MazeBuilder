#include <random>
#include <memory>
#include <exception>
#include <iostream>
#include <sstream>
#include <future>
#include <thread>
#include <shared_mutex>
#include <algorithm>
#include <vector>
#include <list>

#include "grid.h"
#include "args_builder.h"
#include "maze_types_enum.h"
#include "maze_factory.h"
#include "maze_thread_safe.h"
#include "writer.h"
#include "craft.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

// bind a getter method from C++ so that it can be accessed in the frontend with JS
EMSCRIPTEN_BINDINGS(maze_builder_module) {
    emscripten::class_<craft>("craft")
        .smart_ptr<std::shared_ptr<craft>>("std::shared_ptr<craft>")
        .constructor<const std::string&, const std::string&, const std::string&>()
        .function("set_json", &craft::set_json)
        .function("get_json", &craft::get_json)
        .class_function("get_instance", &craft::get_instance, emscripten::allow_raw_pointers());
}
#endif

int main(int argc, char* argv[]) {

#if defined(MAZE_DEBUG)
    static constexpr auto MAZE_BUILDER_VERSION = "maze_builder=[3.10.7] - DEBUG";
#else
    static constexpr auto MAZE_BUILDER_VERSION = "maze_builder=[3.10.7]";
#endif

    static constexpr auto HELP_MSG = R"help(
        Usages: maze_builder.exe [OPTION(S)]... [OUTPUT]
        Generates mazes and exports to ASCII-format or Wavefront object format
        Example: maze_builder.exe -w 10 -l 10 -a binary_tree > out_maze.txt
          -a, --algorithm    binary_tree [default], sidewinder
          -s, --seed         seed for the random number generator [mt19937]
          -w, --width        maze width [default=100]
          -y, --height       maze height [default=10]
          -l, --length       maze length [default=100]
          -i, --interactive  run program in interactive mode with a GUI
          -o, --output       stdout [default], plain text [.txt], or Wavefront object format [.obj]
          -h, --help         display this help message
          -v, --version      display program version
    )help";

    std::vector<std::string> args_vec;
    for (int i = 0; i < argc; i++) {
        args_vec.emplace_back(argv[i]);
    }

#if defined(__EMSCRIPTEN__)
    // Force the -i param when compiling to the web
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
#if defined(MAZE_DEBUG)
    for (auto&& [k, v] : args.build()) {
            std::cout << "INFO: " << k << ", " << v << "\n";
    }
#endif
    
    if (state_of_args == mazes::args_state::JUST_NEEDS_HELP) {
        std::cout << HELP_MSG << std::endl;
        return EXIT_SUCCESS;
    } else if (state_of_args == mazes::args_state::JUST_NEEDS_VERSION) {
        std::cout << MAZE_BUILDER_VERSION << std::endl;
        return EXIT_SUCCESS;
    }

    auto user_seed_or_not = static_cast<unsigned long>(args.get_seed());
    auto seed_as_ul = user_seed_or_not;
    std::mt19937 rng_engine{ seed_as_ul };
    auto get_int = [&rng_engine](int low, int high) -> int {
        using namespace std;
        uniform_int_distribution<int> dist {low, high};
        return dist(rng_engine);
    };

    std::list<std::string> algos = { "binary_tree", "sidewinder" };
    auto get_maze_type_from_algo = [](const std::string& algo)->mazes::maze_types {
        using namespace std;
        if (algo.compare("binary_tree") == 0) {
            return mazes::maze_types::BINARY_TREE;
        } else if (algo.compare("sidewinder") == 0) {
            return mazes::maze_types::SIDEWINDER;
        } else {
            return mazes::maze_types::INVALID_ALGO;
        }
    };

    try {
        bool success = false;
        if (args.is_interactive()) {
            // Run the SDL app
            std::string window_title {"Maze Builder"};
            std::string version { MAZE_BUILDER_VERSION };
            std::string help { HELP_MSG };
            auto&& maze_builder_3D = craft::get_instance(window_title, version, help);
            // craft uses it's own RNG engine, which looks a lot like the one here
            success = maze_builder_3D->run(seed_as_ul, std::cref(algos), std::cref(get_maze_type_from_algo));
            if (!success) {
                std::cout << "ERROR: Running SDL app failed." << std::endl;
            }
        } else {
            // Run the command-line program
            mazes::maze_types my_maze_type = get_maze_type_from_algo(args.get_algorithm());
            maze_thread_safe my_maze{ my_maze_type, std::cref(get_int), std::cref(rng_engine),
                args.get_width(), args.get_length(), args.get_height() };
            auto&& maze_str = my_maze.get_maze();
            if (!maze_str.empty()) {
                mazes::writer my_writer;
                auto write_func = [&my_writer, &args](auto data)->bool {
                    return my_writer.write(args.get_output(), data);
                };
                bool is_wavefront_file = (my_writer.get_filetype(args.get_output()) == mazes::file_types::WAVEFRONT_OBJ_FILE);
                if (is_wavefront_file) {
                    success = write_func(my_maze.to_wavefront_obj_str());
                } else {
                    success = write_func(maze_str);
                }
                
                if (success) {
#if defined(MAZE_DEBUG)
                    std::cout << "INFO: Writing to file: " << args.get_output() << " complete!!" << std::endl;
#endif
                }
                else {
                    std::cerr << "ERROR: Writing to file: " << args.get_output() << std::endl;
                }
            }
            else {
                std::cerr << "ERROR: " << args.get_algorithm() << " failed!!" << std::endl;
            }
        }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
}
