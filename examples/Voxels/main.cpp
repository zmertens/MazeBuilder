// Main file for the Maze Builder voxel editor application.

#include <iostream>
#include <stdexcept>
#include <memory>
#include <string>

#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/randomizer.h>

#include "craft.h"

// Run the SDL app
static constexpr auto window_w = 1200, window_h = 800;

const auto title{"Maze Builder - " + mazes::buildinfo::Version};

// Avoid function-local static initialization on wasm main thread.
// Eager init sidesteps __cxa_guard_acquire/pthread_cond_wait warnings.
std::shared_ptr<craft> g_voxel_engine = std::make_shared<craft>(title, window_w, window_h);

// Setup for Emscripten/WebAssembly
// Bind a getter method from C++ so that it can be accessed in the frontend with JS
#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

std::shared_ptr<craft> get()
{
    return g_voxel_engine;
}

EMSCRIPTEN_BINDINGS (craft_module)
{
    emscripten::function("get", &get, emscripten::allow_raw_pointers());
    emscripten::class_<craft>("craft")
        .smart_ptr<std::shared_ptr<craft>>("std::shared_ptr<craft>")
        .constructor<const std::string&, int, int>()
        .function("artifacts", &craft::artifacts)
        .function("is_download_ready", &craft::is_download_ready)
        .function("reset_download_flag", &craft::reset_download_flag);
}
#endif

int main()
{
    try
    {
        mazes::randomizer rng;

        if (!g_voxel_engine->run(nullptr, std::ref(rng)))
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
