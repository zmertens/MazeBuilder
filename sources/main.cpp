#include <random>
#include <memory>
#include <exception>
#include <iostream>
#include <sstream>
#include <future>
#include <thread>
#include <algorithm>
#include <vector>
#include <list>

#include "grid.h"
#include "args_builder.h"
#include "maze_types_enum.h"
#include "maze_factory.h"
#include "writer.h"
#include "craft.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>
    // bind a getter method from C++ so that it can be accessed in the frontend with JS
    EMSCRIPTEN_BINDINGS(maze_builder_module) {
        emscripten::class_<craft>("craft")
            .constructor<std::string, std::string, std::string>()
            .function("is_json_rdy", &craft::is_json_rdy)
            .function("get_json", &craft::get_json);
   }
#endif

int main(int argc, char* argv[]) {

#if defined(MAZE_DEBUG)
    static constexpr auto MAZE_BUILDER_VERSION = "maze_builder=[3.1.5] - DEBUG";
#else
    static constexpr auto MAZE_BUILDER_VERSION = "maze_builder=[3.1.5]";
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
    
    if (state_of_args == mazes::args_state::JUST_NEEDS_HELP) {
        std::cout << HELP_MSG << std::endl;
        return EXIT_SUCCESS;
    } else if (state_of_args == mazes::args_state::JUST_NEEDS_VERSION) {
        std::cout << MAZE_BUILDER_VERSION << std::endl;
        return EXIT_SUCCESS;
    }

    auto seed_as_ul = std::stoul(args_map.at("seed"));
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
            
            std::string window_title {"Maze Builder"};
            std::string version { MAZE_BUILDER_VERSION };
            std::string help { HELP_MSG };
            craft maze_builder_3D {window_title, version, help };
            // craft uses it's own RNG engine, which looks a lot like the one here
            success = maze_builder_3D.run(seed_as_ul, std::cref(algos), std::cref(get_maze_type_from_algo));
        } else {
            mazes::maze_types my_maze_type = get_maze_type_from_algo(args.get_algorithm());
            auto _grid{ std::make_unique<mazes::grid>(args.get_width(), args.get_length(), args.get_height()) };
            
            success = mazes::maze_factory::gen_maze(my_maze_type, std::ref(_grid), std::cref(get_int), std::cref(rng_engine));
            if (success) {
                mazes::writer my_writer;
                auto write_func = [&my_writer, &args](auto data)->bool {
                    return my_writer.write(args.get_output(), data);
                    };
                std::packaged_task<bool(const std::string& data)> task_writes(write_func);

                auto&& fut_writer = task_writes.get_future();
                std::stringstream ss;
                ss << *_grid.get();
                std::thread thread_writer(std::move(task_writes), ss.str());
                thread_writer.join();
                if (fut_writer.get()) {
#if defined(MAZE_DEBUG)
                    std::cout << "Writing to file: " << args.get_output() << " complete!!" << std::endl;
#endif
                }
                else {
                    std::cerr << "ERROR: Writing to file " << args.get_output() << std::endl;
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
