/// @file main.cpp
/// @brief Main entry point for the maze builder CLI application
/// @details This application generates mazes based on command line arguments
/// @details It supports various algorithms and output formats
/// @details The application can also be compiled to WebAssembly for use in web applications
/// @author zmertens

#include <cstdint>
#include <iostream>
#include <functional>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

#include <MazeBuilder/bytes.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder/string_utils.h>

#include "cli.h"

#if defined(__EMSCRIPTEN__)

#include <emscripten/bind.h>

std::shared_ptr<cli> get()
{
    return mazes::singleton_base<cli>::instance();
}

EMSCRIPTEN_BINDINGS (cli_module)
{
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

int main(const int argc, char* argv[])
{
#if defined(__EMSCRIPTEN__)

    return EXIT_SUCCESS;
#endif

    // Copy command arguments and skip the program name
    const std::vector<std::string> args_vec{argv + 1, argv + argc};

    try
    {
        if (const auto my_cli = mazes::singleton_base<cli>::instance())
        {
            mazes::configurator user_options;
            if (const auto str = my_cli->convert_with_options(std::cref(args_vec),
                std::ref(user_options));
                !str.empty())
            {
                std::stringstream stream;
                bool write_success{false};
                constexpr mazes::io_utils writer{};

                // PNG and JPEG formats handle their own file writing in create.h
                if (const auto output_fmt = user_options.output_format_id();
                    output_fmt == mazes::output_format::PNG || output_fmt == mazes::output_format::JPEG)
                {
                    const auto filename = user_options.output_filename();
                    if (user_options.output_format_id() == mazes::output_format::PNG)
                    {
                        write_success = writer.write_png(filename,
                                                         mazes::bytes::string_to_bytes(str),
                                                         user_options.image_width(),
                                                         user_options.image_height(),
                                                         4);
                    }
                    else if (user_options.output_format_id() == mazes::output_format::JPEG)
                    {
                        write_success = writer.write_jpeg(filename, mazes::bytes::string_to_bytes(str),
                                                          user_options.image_width(),
                                                          user_options.image_height(),
                                                          4);
                    }

                    stream << "Wrote file: " << filename << std::endl;
                    stream << "Image dimensions: " << user_options.image_width() << "x"
                        << user_options.image_height() << std::endl;
                }
                else
                {
                    // Check if we have a specific output filename
                    if (const auto filename = user_options.output_filename(); !filename.empty())
                    {
                        if (user_options.output_format_id() == mazes::output_format::STDOUT)
                        {
                            // Write to stdout
                            write_success = writer.write(std::cout, str);
                            stream << "Wrote to standard output." << std::endl;
                        }
                        else
                        {
                            // Write to file
                            write_success = writer.write_file(user_options.output_filename(), str);
                            stream << "Wrote file: " << filename << std::endl;
                        }
                    }
                    else
                    {
                        write_success = writer.write(std::cout, str);
                        stream << "Wrote to standard output." << std::endl;
                    }
                }

                if (!write_success)
                {
                    throw std::runtime_error("Failed to write output.");
                }

#if defined(MAZE_DEBUG)

                std::cout << stream.str() << std::endl;
#endif
            }
            else
            {
                throw std::logic_error(str);
            }
        }
        else
        {
            throw std::runtime_error(my_cli->help());
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }

    return EXIT_SUCCESS;
} // main
