/// @file main.cpp
// @brief Main entry point for the maze builder CLI application
// @details This application generates mazes based on command line arguments
// @details It supports various algorithms and output formats
// @details The application can also be compiled to WebAssembly for use in web applications
// @author zmertens

#include <iostream>
#include <functional>
#include <stdexcept>
#include <vector>

#include "cli.h"

#if defined(__EMSCRIPTEN__)

#include <emscripten/bind.h>

std::shared_ptr<cli> get() {

    return mazes::singleton_base<cli>::instance();
}

EMSCRIPTEN_BINDINGS(cli_module) {
    emscripten::function("get", &get, emscripten::allow_raw_pointers());
    emscripten::class_<cli>("cli")
        .smart_ptr<std::shared_ptr<cli>>("std::shared_ptr<cli>")
        .function("run", &cli::run);
}

#endif // EMSCRIPTEN_BINDINGS

int main(int argc, char* argv[]) {

#if defined(__EMSCRIPTEN__)

    return EXIT_SUCCESS;
#endif

    using namespace std;

    // Copy command arguments and skip the program name
    vector<string> args_vec{ argv + 1, argv + argc };

    // Debug: print the arguments we received
    cerr << "DEBUG: Received " << args_vec.size() << " arguments:" << endl;
    for (size_t i = 0; i < args_vec.size(); ++i) {
        cerr << "  args[" << i << "] = \"" << args_vec[i] << "\"" << endl;
    }

    try {

        cli my_cli;

        if (auto str = my_cli.convert(cref(args_vec)); !str.empty()) {

            cout << str << endl;
        } else {

            throw runtime_error("Failed to parse command line arguments.");
        }


    } catch (std::exception& ex) {

        std::cerr << ex.what() << std::endl;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
} // main
