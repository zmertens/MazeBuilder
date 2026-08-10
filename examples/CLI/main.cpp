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
#include <unordered_map>
#include <vector>

#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/singleton_base.h>
#include <MazeBuilder/string_utils.h>

class command_line_parser : public mazes::singleton_base<command_line_parser>
{
    enum class HelpPartsIndex : unsigned int
    {
        ALGOS = 0,
        OUTPUT_FORMATS = 1,
        TOTAL = 2
    };

    std::unordered_map<HelpPartsIndex, std::string> help_parts;

public:
    command_line_parser() noexcept
    {
        append_algos_and_set();
        append_outputs_and_set();
    }

    std::string version() noexcept
    {
        return mazes::string_utils::concat(mazes::string_utils::concat("v", mazes::buildinfo::VERSION),
                                           " - " + mazes::buildinfo::COMMIT_SHA);
    }

    void append_algos_and_set() noexcept
    {
        std::string result;
        for (auto algo = 0; algo < static_cast<unsigned int>(mazes::algo::TOTAL); ++algo)
        {
            if (!result.empty())
            {
                result += ", ";
            }
            result += std::string{mazes::to_sv_from_algo(static_cast<mazes::algo>(algo))};
        }
        help_parts.insert_or_assign(HelpPartsIndex::ALGOS, result);
    }

    void append_outputs_and_set() noexcept
    {
        std::string result;
        for (auto output = 0; output < static_cast<unsigned int>(mazes::output_format::TOTAL); ++output)
        {
            if (!result.empty())
            {
                result += ", ";
            }
            result += std::string{mazes::to_sv_from_output_format(static_cast<mazes::output_format>(output))};
        }
        help_parts.insert_or_assign(HelpPartsIndex::OUTPUT_FORMATS, result);
    }

    std::string help() noexcept
    {
        return "mazebuildercli " + version() + "\n\n" +
               "Generates mazes and converts to various formats\n\n"
               "Example: mazebuildercli -r 14 -c 10 -a binary_tree -o stdout\n\n"
               "Example: mazebuildercli --rows=5 --columns=6 --algo=dfs --output=maze.obj\n\n"
               "Example: mazebuildercli -r 20 -c 20 -a sidewinder -o maze.png\n\n"
               "** Commands are case-sensitive! **\n\n"
               "\t-a, --algo         algorithm to apply to maze links\n"
               "\t                     [" + help_parts.at(HelpPartsIndex::ALGOS) + "]\n"
               "\t-c, --columns      columns [max: " + std::to_string(mazes::configurator::MAX_COLUMNS) + "]\n"
               "\t-d, --distances    show distances with optional [start, end] inclusive\n"
               "\t                     example: '-d [0:10]'\n"
               "\t-h, --help         display this help message\n"
               "\t-j, --json         run with arguments in JSON format\n"
               "\t-l, --levels       levels [max: " + std::to_string(mazes::configurator::MAX_LEVELS) + "]\n"
               "\t-m, --mask         load mask from text file\n"
               "\t-s, --seed         seed for the number generator\n"
               "\t-r, --rows         rows [max: " + std::to_string(mazes::configurator::MAX_ROWS) + "]\n"
               "\t-o, --output       output format [" + help_parts.at(HelpPartsIndex::OUTPUT_FORMATS) + "]\n"
               "\t-v, --version      display program version\n";
    }

    std::string run(const std::string &arguments) noexcept
    {
        if (auto maze = mazes::runtime_app::instance())
        {
            return std::string{maze->apply(arguments)};
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

    auto &&maze = mazes::runtime_app::instance();

    auto &&logger = mazes::global_async_logger();
    std::vector<std::string> logs;
    logs.reserve(100);
    logger.set_sink([&logs](std::string_view msg)
    {
        logs.emplace_back(msg);
        std::cerr << msg << std::endl;
    });

    // Copy command arguments and skip the program name
    const std::vector<std::string> args_vec{argv + 1, argv + argc};

    try
    {
        if (args_vec.empty() || find_str(args_vec, mazes::args::HELP_FLAG_STR) 
            || find_str(args_vec, mazes::args::HELP_OPTION_STR))
        {
            logger.log_message(parser->help());
        }
        if (find_str(args_vec, mazes::args::VERSION_FLAG_STR) || find_str(args_vec, mazes::args::VERSION_OPTION_STR))
        {
            logger.log_message(parser->version());
        }
        else
        {
            std::string concatenated_args;
            for (const auto &arg : args_vec)
            {
                concatenated_args += arg + " ";
            }

            if (auto &&results = maze->apply(concatenated_args); !results.empty())
            {
                logger.log_message(std::string{results});
            }
            else
            {
                logger.log_message("No output generated from the provided arguments.");
            }
        }
    }
    catch (const std::exception &ex)
    {
        logger.log_message(ex.what());
        return EXIT_FAILURE;
    }

    std::for_each(logs.cbegin(), logs.cend(), [](const std::string &msg)
    {
        std::cerr << mazes::string_utils::format("{}", msg) << std::endl;
    });

    logger.flush();

    return EXIT_SUCCESS;
} // main
