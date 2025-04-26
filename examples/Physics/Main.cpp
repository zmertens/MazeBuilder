/**
 * @brief Main entry for the Generator
 * @file Main.cpp
 *
 * Create a singleton instance of the Generator and run it
 *
 */

#include <iostream>
#include <exception>
#include <string>

#include "Physics.hpp"

#include <MazeBuilder/maze_builder.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

// This is necessary due to emscripten's handling of templates
std::shared_ptr<Physics> get() {
    std::string title = "physics - gamedev.js jam 2025";
    std::string version = "0.1.0";
    return mazes::singleton_base<Physics>::instance(cref(title), cref(version), 1280, 720);
}

// bind a getter method from C++ so that it can be accessed in the frontend with JS
EMSCRIPTEN_BINDINGS(maze_builder_module) {
    emscripten::function("get", &get, emscripten::allow_raw_pointers());
    emscripten::class_<Physics>("Physics")
        .smart_ptr<std::shared_ptr<Physics>>("std::shared_ptr<Physics>")
        .constructor<const std::string&, const std::string&, int, int>();
}
#endif

int main(int argc, char* argv[]) {
	using namespace std;

    static constexpr auto MESSAGE = R"msg(
        --- WELCOME TO THE MAZE GENERATOR ---
        |   1. Press 'B' on keyboard to generate a 2D maze   |
        -------------------------------------------
    )msg";

    cout << MESSAGE << endl;

    try {
        string title = "physics - gamedev.js jam 2025";
        string version = "0.1.0";
        auto myGameInstance = mazes::singleton_base<Physics>::instance(cref(title), cref(version), 1280, 720);
        bool res = myGameInstance->run();
        if (!res) {
            cerr << "Generator failed to run" << endl;
        }
    } catch (exception ex) {
        cerr << ex.what() << endl;
    }

	return 0;
}
