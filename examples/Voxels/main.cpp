#include <random>
#include <exception>
#include <iostream>
#include <algorithm>
#include <string>

#include <MazeBuilder/buildinfo.h>

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

    std::mt19937 rng_engine{ static_cast<unsigned long>(100) };
    auto get_int = [&rng_engine](int low, int high) -> int {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng_engine);
    };

    try {
#if !defined(__EMSCRIPTEN__)
        bool success = false;
        // Run the SDL app
        static constexpr int window_w = 800, window_h = 600;
        string_view my_title { "Maze Builder 🔧" };
        auto&& maze_builder_3D = craft::get_instance(cref(my_title), mazes::build_info::Version, window_w, window_h);
        success = maze_builder_3D->run(std::cref(get_int), std::ref(rng_engine));
        if (!success) {
            std::cerr << "ERROR: Running SDL app failed." << std::endl;
        }
#endif
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
}
