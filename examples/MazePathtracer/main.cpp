// MazePathtracer - entry point
//
// Generates a 2D maze with MazeBuilder and renders it in 3D using software
// path tracing.  Wall cells become spheres in the scene; the result is
// uploaded to the GPU via SDL3's SDL_GPU API and displayed in an SDL window.
//
// Controls:
//   SPACE / R  →  generate and render a new random maze
//   ESC / Q    →  quit

#include <iostream>
#include <exception>
#include <string>

#include "pathtracer_app.h"

#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/singleton_base.h>

static std::string TITLE_STR   = "Maze Builder - Path Tracer";
static std::string VERSION_STR = "v0.1.0";

static constexpr int WINDOW_W = 1280;
static constexpr int WINDOW_H = 720;

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

std::shared_ptr<pathtracer_app> get()
{
    return mazes::singleton_base<pathtracer_app>::instance(
        std::cref(TITLE_STR), std::cref(VERSION_STR), WINDOW_W, WINDOW_H);
}

EMSCRIPTEN_BINDINGS(maze_pathtracer_module)
{
    emscripten::function("get", &get, emscripten::allow_raw_pointers());
    emscripten::class_<pathtracer_app>("pathtracer_app")
        .smart_ptr<std::shared_ptr<pathtracer_app>>("std::shared_ptr<pathtracer_app>")
        .constructor<const std::string&, const std::string&, int, int>();
}
#endif

int main()
{
    using std::cerr;
    using std::endl;
    using std::exception;
    using std::runtime_error;

    using mazes::randomizer;
    using mazes::singleton_base;

#if defined(MAZE_DEBUG)
    VERSION_STR += " - DEBUG";
#endif

    try
    {
        const auto inst = singleton_base<pathtracer_app>::instance(
            TITLE_STR, VERSION_STR, WINDOW_W, WINDOW_H);

        randomizer rng;
        if (!inst->run(nullptr, rng))
        {
            throw runtime_error("Error: " + TITLE_STR +
                                " encountered an error during execution");
        }
    }
    catch (const exception& ex)
    {
        cerr << ex.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
