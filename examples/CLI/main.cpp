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
    emscripten::function("get", &get, emscripten::allow_raw_pointers());
    emscripten::class_<cli>("cli")
        .smart_ptr<std::shared_ptr<cli>>("std::shared_ptr<cli>")
        .function("convert", &cli::convert);

    emscripten::register_vector<std::string>("std::vector<std::string>");
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
        
        if (config) {

            mazes::writer writer;

            bool write_success = false;
            
            // Check if we have a specific output filename
            if (!config->output_filename().empty()) {

                if (mazes::to_output_from_string(config->output_filename()) == mazes::output::STDOUT) {

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
        } else {

            // No configuration available, just write to stdout
            cout << str << endl;
        }

    } catch (const std::exception& ex) {

        std::cerr << ex.what() << std::endl;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
} // main
