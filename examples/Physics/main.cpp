// Basic application of Maze Builder as a level generator in a game setting
// Includes most game engine features like graphics and window management,
// input handling, state management, and resource loading, audio, and network
// Player verses computer AI gameplay with physics simulation
// Scoring system based on survivability (time) and efficiency (resources)

#include <iostream>
#include <exception>
#include <string>

#include "physics_game.h"

#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/singleton_base.h>
#include <MazeBuilder/string_utils.h>

static std::string TITLE_STR = "Maze Builder - Physics Example";

static std::string VERSION_STR = "v0.3.1";

static constexpr auto WINDOW_W = 1280;
static constexpr auto WINDOW_H = 720;

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

std::shared_ptr<physics_game> get()
{
    return mazes::singleton_base<physics_game>::instance(cref(TITLE_STR), cref(VERSION_STR), WINDOW_W, WINDOW_H);
}

// bind a getter method from C++ so that it can be accessed in the frontend with JS
EMSCRIPTEN_BINDINGS (maze_builder_module)
{
    emscripten::function("get", &get, emscripten::allow_raw_pointers());
    emscripten::class_<physics_game>("physics_game")
        .smart_ptr<std::shared_ptr<physics_game>>("std::shared_ptr<physics_game>")
        .constructor<const std::string&, const std::string&, int, int>();
}
#endif

int main(int argc, char* argv[])
{
    using std::cerr;
    using std::cout;
    using std::cref;
    using std::endl;
    using std::exception;
    using std::ref;
    using std::runtime_error;
    using std::string;

    using mazes::randomizer;
    using mazes::singleton_base;
    using mazes::string_utils;

#if defined(MAZE_DEBUG)

    VERSION_STR += " - DEBUG";
#endif

    try
    {
        const auto inst = singleton_base<physics_game>::instance(TITLE_STR, VERSION_STR, WINDOW_W, WINDOW_H);

        if (randomizer rng; !inst->run(nullptr, ref(rng)))
        {
            throw runtime_error("Error: " + TITLE_STR + " encountered an error during execution");
        }

#if defined(MAZE_DEBUG)

        cout << TITLE_STR << " ran successfully (DEBUG MODE)" << endl;
#endif
    }
    catch (exception& ex)
    {
        cerr << ex.what() << endl;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
