#include <random>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <string>

#include <MazeBuilder/maze_builder.h>

#include "craft.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

// bind a getter method from C++ so that it can be accessed in the frontend with JS
EMSCRIPTEN_BINDINGS(maze_builder_module) {
    emscripten::class_<craft>("craft")
        .smart_ptr<std::shared_ptr<craft>>("std::shared_ptr<craft>")
        .constructor<const std::string&, const std::string&, int, int>()
        .function("mazes", &craft::mazes)
        .function("toggle_mouse", &craft::toggle_mouse)
        .class_function("get_instance", &craft::get_instance, emscripten::allow_raw_pointers());
}
#endif

int main(int argc, char* argv[]) {

    using namespace std;

    mazes::randomizer rng;

    try {

        bool success = false;
        // Run the SDL app
        static constexpr int window_w = 800, window_h = 600;
        string my_title { "Maze Builder 🔧" };
        auto&& voxel_engine = craft::get_instance(cref(my_title), mazes::VERSION, window_w, window_h);
        success = voxel_engine->run(std::ref(rng));
        if (!success) {
            std::cerr << "ERROR: Running SDL app failed." << std::endl;
        }

    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
}
