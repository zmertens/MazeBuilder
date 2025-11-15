// Basic application of Maze Builder as a level generator in a game setting
// Includes most game engine features like graphics and window management,
// input handling, state management, and resource loading, audio, and network
// Player verses computer AI gameplay with physics simulation
// Scoring system based on survivability (time) and efficiency (resources)

#include <iostream>
#include <exception>
#include <string>

#include "PhysicsGame.hpp"

#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/singleton_base.h>
#include <MazeBuilder/string_utils.h>

static std::string TITLE_STR = "Breaking Walls";

static std::string VERSION_STR = "v0.3.0";

static constexpr auto WINDOW_W = 1280;
static constexpr auto WINDOW_H = 720;

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

std::shared_ptr<PhysicsGame> get()
{
    return mazes::singleton_base<PhysicsGame>::instance(cref(TITLE_STR), cref(VERSION_STR), WINDOW_W, WINDOW_H);
}

// bind a getter method from C++ so that it can be accessed in the frontend with JS
EMSCRIPTEN_BINDINGS (maze_builder_module)
{
    emscripten::function("get", &get, emscripten::allow_raw_pointers());
    emscripten::class_<PhysicsGame>("PhysicsGame")
        .smart_ptr<std::shared_ptr<PhysicsGame>>("std::shared_ptr<PhysicsGame>")
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

    string configPath{};

#if !defined(__EMSCRIPTEN__)

    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <path_to_config.json>" << endl;

        return EXIT_FAILURE;
    }

    if (!string_utils::contains(argv[1], ".json"))
    {
        cerr << "Error: Configuration file must be a .json file" << endl;

        return EXIT_FAILURE;
    }

    configPath = argv[1];
#else

    configPath = "resources/physics.json";
#endif

    try
    {
        const auto inst = singleton_base<PhysicsGame>::instance(TITLE_STR, VERSION_STR, configPath, WINDOW_W, WINDOW_H);

        if (randomizer rng; !inst->run(nullptr, ref(rng)))
        {
            throw runtime_error("Error: PhysicsGame encountered an error during execution");
        }

#if defined(MAZE_DEBUG)

        cout << "PhysicsGame ran successfully (DEBUG MODE)" << endl;
#endif
    }
    catch (exception& ex)
    {
        cerr << ex.what() << endl;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
