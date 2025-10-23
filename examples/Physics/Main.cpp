// Basic application of Maze Builder as a level generator (with bouncing balls!)

#include <iostream>
#include <exception>
#include <string>

#include "PhysicsGame.hpp"

#include <MazeBuilder/maze_builder.h>

static std::string TITLE_STR = "Breaking Walls";

static std::string VERSION_STR = "v0.1.9";

static constexpr auto WINDOW_W = 1280;
static constexpr auto WINDOW_H = 720;

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

std::shared_ptr<PhysicsGame> get() {

    return mazes::singleton_base<PhysicsGame>::instance(cref(TITLE_STR), cref(VERSION_STR), WINDOW_W, WINDOW_H);
}

// bind a getter method from C++ so that it can be accessed in the frontend with JS
EMSCRIPTEN_BINDINGS(maze_builder_module) {
    emscripten::function("get", &get, emscripten::allow_raw_pointers());
    emscripten::class_<PhysicsGame>("PhysicsGame")
        .smart_ptr<std::shared_ptr<PhysicsGame>>("std::shared_ptr<PhysicsGame>")
        .constructor<const std::string&, const std::string&, int, int>();
}
#endif

int main(int argc, char* argv[]) {

    using std::cerr;
    using std::cout;
    using std::cref;
    using std::endl;
    using std::exception;
    using std::runtime_error;

#if defined(MAZE_DEBUG)

    VERSION_STR += " - DEBUG";
#endif

    try {

        using mazes::singleton_base;

        if (auto inst = singleton_base<PhysicsGame>::instance(cref(TITLE_STR), cref(VERSION_STR), WINDOW_W, WINDOW_H); inst->run()) {

#if defined(MAZE_DEBUG)

            cout << "PhysicsGame ran successfully (DEBUG MODE)" << endl;
#endif
        } else {

            throw runtime_error("Error: PhysicsGame encountered an error during execution");
        }

    } catch (exception ex) {

        cerr << ex.what() << endl;
    }

	return 0;
}
