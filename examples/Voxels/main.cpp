#include <iostream>
#include <stdexcept>
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

int main() {

    try {
        mazes::randomizer rng;

        // Run the SDL app
        static constexpr int window_w = 1080, window_h = 720;

        const auto title{ "Maze Builder 🔧 " + mazes::VERSION };

        if (const auto voxel_engine = mazes::singleton_base<craft>::instance(title, window_w, window_h);
            !voxel_engine->run(nullptr, std::ref(rng))) {

            throw std::runtime_error("ERROR: Running SDL app failed.");
        }

    } catch (std::exception& ex) {

        std::cerr << ex.what() << std::endl;
    }

    return EXIT_SUCCESS;
}
