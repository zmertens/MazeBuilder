#include <random>
#include <memory>
#include <exception>
#include <iostream>
#include <sstream>
#include <future>
#include <thread>
#include <shared_mutex>
#include <algorithm>
#include <string>
#include <vector>
#include <list>

#include <MazeBuilder/distances.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/args_builder.h>
#include <MazeBuilder/output_types_enum.h>
#include <MazeBuilder/maze_factory.h>
#include <MazeBuilder/maze_builder.h>
#include <MazeBuilder/writer.h>

#include "craft.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

// bind a getter method from C++ so that it can be accessed in the frontend with JS
EMSCRIPTEN_BINDINGS(maze_builder_module) {
    emscripten::class_<craft>("craft")
        .smart_ptr<std::shared_ptr<craft>>("std::shared_ptr<craft>")
        .constructor<const std::string&, const std::string&, int, int>()
        .function("fullscreen", &craft::fullscreen)
        .function("mouse", &craft::mouse)
        .function("mazes", &craft::mazes)
        .class_function("get_instance", &craft::get_instance, emscripten::allow_raw_pointers());
}
#endif

int main(int argc, char* argv[]) {

    using namespace std;

    std::mt19937 rng_engine{ static_cast<unsigned long>(100) };
    auto get_int = [&rng_engine](int low, int high) -> int {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng_engine);
    };

    // Convert algorithm string into an enum type
    std::list<std::string> algos = { "binary_tree", "sidewinder", "dfs" };
    auto get_maze_type_from_algo = [](const std::string& algo)->mazes::maze_types {
        if (algo.compare("binary_tree") == 0) {
            return mazes::maze_types::BINARY_TREE;
        } else if (algo.compare("sidewinder") == 0) {
            return mazes::maze_types::SIDEWINDER;
        } else if (algo.compare("dfs") == 0) {
            return mazes::maze_types::DFS;
        } else {
            return mazes::maze_types::INVALID_ALGO;
        }
    };

    try {
        bool success = false;
        // Run the SDL app
        static constexpr int window_w = 800, window_h = 600;
        auto&& maze_builder_3D = craft::get_instance("", "", window_w, window_h);
        success = maze_builder_3D->run(std::cref(algos), std::cref(get_maze_type_from_algo), std::cref(get_int), std::ref(rng_engine));
        if (!success) {
            std::cerr << "ERROR: Running SDL app failed." << std::endl;
        }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
}
