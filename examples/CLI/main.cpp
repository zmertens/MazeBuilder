/// @file main.cpp
/// @brief Main entry point for the maze builder CLI application
/// @details This application generates mazes based on command line arguments
/// @details It supports various algorithms and output formats
/// @details The application can also be compiled to WebAssembly for use in web applications
/// @author zmertens

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/singleton_base.h>
#include <MazeBuilder/string_utils.h>

class command_line_parser : public mazes::singleton_base<command_line_parser>
{
public:
    std::string version() noexcept
    {
        return mazes::string_utils::concat(mazes::string_utils::concat("v", mazes::buildinfo::VERSION),
                                           " - " + mazes::buildinfo::COMMIT_SHA);
    }

    std::string help() noexcept
    {
        return "mazebuildercli " + version() + "\n\n" +
               "Generates mazes and converts to various formats\n\n"
               "Example: mazebuildercli -r 14 -c 10 -a binary_tree -o stdout\n\n"
               "Example: mazebuildercli --rows=5 --columns=6 --algo=dfs --output=maze.obj\n\n"
               "Example: mazebuildercli -r 20 -c 20 -a sidewinder -o maze.png\n\n"
               "** Commands are case-sensitive! **\n\n"
               "\t-a, --algo         algorithm to generate maze links\n"
               "\t                     [binary_tree, dfs, sidewinder]\n"
               "\t-c, --columns      columns [max: 100]\n"
               "\t-d, --distances    show distances with optional [start, end] inclusive\n"
               "\t                     example: '-d [0:10]'\n"
               "\t-h, --help         display this help message\n"
               "\t-H, --image-height output image height in pixels (reserved)\n"
               "\t-j, --json         run with arguments in JSON format\n"
               "\t-l, --levels       levels [max: 10]\n"
               "\t-m, --mask         load mask from text file\n"
               "\t-S, --show-steps   emit step snapshots during generation\n"
               "\t-s, --seed         seed for the number generator\n"
               "\t-r, --rows         rows [max: 100]\n"
               "\t-W, --image-width  output image width in pixels (reserved)\n"
               "\t-o, --output       output format [json, obj, txt, png, jpg, jpeg, bmp, sfml, stdout]\n"
               "\t-v, --version      display program version\n";
    }

    std::string run(const std::string &arguments) noexcept
    {
        if (auto app = mazes::runtime_app::instance())
        {
            return std::string{app->apply(arguments)};
        }
        return {};
    }
}; // class

std::shared_ptr<command_line_parser> parser = std::make_shared<command_line_parser>();

#if defined(__EMSCRIPTEN__)

#include <emscripten/bind.h>

std::shared_ptr<command_line_parser> get()
{
    return parser;
}

EMSCRIPTEN_BINDINGS(cli_module)
{
    emscripten::function("get", &get);
    emscripten::class_<command_line_parser>("cli")
        .smart_ptr<std::shared_ptr<command_line_parser>>("shared_ptr<command_line_parser>")
        .function("help", &command_line_parser::help)
        .function("version", &command_line_parser::version)
        .function("run", &command_line_parser::run);

    emscripten::register_vector<std::string>("StringVector");
}

#endif // EMSCRIPTEN_BINDINGS

int main(const int argc, char *argv[])
{
#if defined(__EMSCRIPTEN__)

    return EXIT_SUCCESS;
#endif

    auto find_str = [](const std::vector<std::string> &vec, const std::string &target) -> bool
    {
        return std::find(vec.cbegin(), vec.cend(), target) != vec.cend();
    };

    auto find_output_target = [](const std::vector<std::string> &vec) -> std::string
    {
        for (std::size_t i = 0; i < vec.size(); ++i)
        {
            const auto &arg = vec[i];
            if (arg == "-o" || arg == "--output")
            {
                if (i + 1 < vec.size())
                {
                    return vec[i + 1];
                }
                break;
            }

            if (arg.rfind("--output=", 0) == 0)
            {
                return arg.substr(std::string{"--output="}.size());
            }
        }

        return {};
    };

    auto should_echo_results = [](std::string_view output_target) -> bool
    {
        if (output_target.empty() || output_target == "stdout" || output_target == "sfml")
        {
            return true;
        }

        return std::filesystem::path{output_target}.extension().empty();
    };

    auto &&app = mazes::runtime_app::instance();
    auto &&logger = mazes::global_async_logger();

    // Copy command arguments and skip the program name
    const std::vector<std::string> args_vec{argv + 1, argv + argc};

    try
    {
        if (const auto my_cli = mazes::singleton_base<command_line_parser>::instance())
        {
            if (args_vec.empty() || find_str(args_vec, "-h") || find_str(args_vec, "--help"))
            {
                logger.log_message(my_cli->help());
            }
            else if (find_str(args_vec, "-v") || find_str(args_vec, "--version"))
            {
                logger.log_message(my_cli->version());
            }
            else
            {
                std::string concatenated_args;
                for (const auto &arg : args_vec)
                {
                    concatenated_args += arg + " ";
                }

                const std::string output_target = find_output_target(args_vec);

                if (auto &&results = app->apply(concatenated_args); !results.empty())
                {
                    if (should_echo_results(output_target) || !output_target.empty())
                    {
                        logger.log_message(std::string{results});
                    }
                }
                else
                {
                    logger.log_message("No output generated from the provided arguments.");
                }
            }
        }
        else
        {
            logger.log_message("Failed to create CLI instance.");
        }
    }
    catch (const std::exception &ex)
    {
        logger.log_message(ex.what());
        return EXIT_FAILURE;
    }

    logger.flush();

    return EXIT_SUCCESS;
} // main
