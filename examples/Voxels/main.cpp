// Main file for the Maze Builder voxel editor application.

#include <iostream>
#include <stdexcept>
#include <string>

#include <MazeBuilder/maze_builder.h>

#include "craft.h"

// Run the SDL app
static constexpr auto window_w = 1080, window_h = 720;

const auto title{"Maze Builder 🔧 " + mazes::VERSION};

// Setup for Emscripten/WebAssembly
// Bind a getter method from C++ so that it can be accessed in the frontend with JS
#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

std::shared_ptr<craft> get()
{
    return mazes::singleton_base<craft>::instance(title, window_w, window_h);
}

EMSCRIPTEN_BINDINGS (craft_module)
{
    emscripten::function("get", &get, emscripten::allow_raw_pointers());
    emscripten::class_<craft>("craft")
        .smart_ptr<std::shared_ptr<craft>>("std::shared_ptr<craft>")
        .constructor<const std::string&, int, int>()
        .function("mazes", &craft::mazes)
        .function("toggle_mouse", &craft::toggle_mouse);
}
#endif

int main()
{
    try
    {
        mazes::randomizer rng;

        if (const auto voxel_engine = mazes::singleton_base<craft>::instance(title, window_w, window_h);
            !voxel_engine->run(nullptr, std::ref(rng)))
        {
            throw std::runtime_error("ERROR: Running SDL app failed.");
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }

    return EXIT_SUCCESS;
}
