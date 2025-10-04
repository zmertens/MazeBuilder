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
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder//string_utils.h>

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
        .function("convert", &cli::convert)
        .function("convert_as_base64", &cli::convert_as_base64)
        .function("help", &cli::help)
        .function("version", &cli::version);

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

        if (auto str = my_cli.convert(cref(args_vec)); !str.empty()) {

            if (auto config = my_cli.get_config(); config != nullptr) {

                mazes::io_utils writer{};

                bool write_success{ false };

                // Check if we have a specific output filename
                if (auto filename = config->output_format_filename(); !filename.empty()) {

                    if (mazes::to_output_format_from_sv(mazes::string_utils::get_file_extension(filename)) == mazes::output_format::STDOUT) {

                        // Write to stdout
                        write_success = writer.write(cout, str);
                    } else {

                        // Write to file
                        write_success = writer.write_file(config->output_format_filename(), str);
                    }
                } else {

                    write_success = writer.write(cout, str);
                }

                if (!write_success) {

                    throw runtime_error("Failed to write output.");
                }
            } else {

                throw logic_error(str);
            }
        } else {

            throw runtime_error(my_cli.help());
        }

    } catch (const std::exception& ex) {

        std::cerr << ex.what() << std::endl;

        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
} // main
