/// @file main.cpp
/// @brief Main entry point for the maze builder CLI application
/// @details This application generates mazes based on command line arguments
/// @details It supports various algorithms and output formats
/// @details The application can also be compiled to WebAssembly for use in web applications
/// @author zmertens

#include <iostream>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/writer.h>

#include "cli.h"

#if defined(__EMSCRIPTEN__)

#include <emscripten/bind.h>

std::shared_ptr<cli> get() {

    return mazes::singleton_base<cli>::instance();
}

EMSCRIPTEN_BINDINGS(cli_module) {
    emscripten::function("get", &get);
    emscripten::class_<cli>("cli")
        .smart_ptr<std::shared_ptr<cli>>("shared_ptr<cli>")
        .function("convert", &cli::convert, emscripten::allow_raw_pointers());

    emscripten::register_vector<std::string>("StringVector");
}

#endif // EMSCRIPTEN_BINDINGS

int main(int argc, char* argv[]) {

#if defined(__EMSCRIPTEN__)

    return EXIT_SUCCESS;
#endif

    using namespace std;

    // Copy command arguments and skip the program name
    vector<string> args_vec{ argv + 1, argv + argc };

    try {

        cli my_cli;

        auto str = my_cli.convert(cref(args_vec));
        
        if (str.empty()) {

            throw runtime_error("Failed to parse command line arguments.");
        }

        // Get the configuration to determine output handling
        auto config = my_cli.get_config();
        
        if (!config) {

            throw logic_error("No valid configuration found after parsing arguments.\nResult:\n" + str);
        }

        mazes::writer writer;

        bool write_success = false;
            
        // Check if we have a specific output filename
        if (!config->output_filename().empty()) {

            if (config->output_filename() == "stdout") {

                // Write to stdout
                write_success = writer.write(cout, str);
            } else {

                // Write to file
                write_success = writer.write_file(config->output_filename(), str);
            }
        } else {
            // No specific filename, check output type
            if (config->output_id() == mazes::output::STDOUT) {

                write_success = writer.write(cout, str);
            } else {

                // Default to stdout if no specific output handling
                write_success = writer.write(cout, str);
            }
        }
            
        if (!write_success) {

            throw runtime_error("Failed to write output.");
        }

    } catch (const std::exception& ex) {

        std::cerr << ex.what() << std::endl;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
} // main
