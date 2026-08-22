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
#include <mutex>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/singleton_base.h>

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
        return "v" + std::string{ mazes::buildinfo::VERSION } + " - " + std::string{ mazes::buildinfo::COMMIT_SHA };
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
            result += std::string{ mazes::to_sv_from_algo(static_cast<mazes::algo>(algo)) };
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
            result += std::string{ mazes::to_sv_from_output_format(static_cast<mazes::output_format>(output)) };
        }
        help_parts.insert_or_assign(HelpPartsIndex::OUTPUT_FORMATS, result);
    }

    std::string help() noexcept
    {
        return "mazebuildercli " + version() + "\n\n" +
            "Generates and converts mazes into simple data formats\n\n"
            "Example: mazebuildercli -r 14 -c 10 -a binary_tree -o stdout\n\n"
            "Example: mazebuildercli --rows=5 --columns=6 --algo=dfs --output=maze.obj\n\n"
            "Example: mazebuildercli -r 20 -c 15 -s1 -o stdout -a prims\n\n"
            "** Commands are case-sensitive! **\n\n"
            "\t-a, --algo         algorithm to apply to maze links\n"
            "\t                     [" +
            help_parts.at(HelpPartsIndex::ALGOS) + "]\n"
            "\t-c, --columns      columns [max: " +
            std::to_string(mazes::configurator::MAX_COLUMNS) + "]\n"
            "\t-d, --distances    show distances with optional [start, end] inclusive\n"
            "\t                     example: '-d [0:10]'\n"
            "\t-h, --help         display this help message\n"
            "\t-j, --json         run with arguments in JSON format\n"
            "\t-l, --levels       levels [max: " +
            std::to_string(mazes::configurator::MAX_LEVELS) + "]\n"
            "\t-m, --mask         load mask from text file\n"
            "\t-s, --seed         seed for the number generator\n"
            "\t-r, --rows         rows [max: " +
            std::to_string(mazes::configurator::MAX_ROWS) + "]\n"
            "\t-o, --output       output format [" +
            help_parts.at(HelpPartsIndex::OUTPUT_FORMATS) + "]\n"
            "\t-v, --version      display program version\n";
    }

    std::string run(const std::string& arguments) noexcept
    {
        if (auto maze = mazes::runtime_app::instance())
        {
            return std::string{ maze->apply(arguments) };
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

int main(const int argc, char* argv[])
{
#if defined(__EMSCRIPTEN__)

    return EXIT_SUCCESS;
#endif

    // Return true on first match, false otherwise
    auto check_for_matches = [](const std::vector<std::string>& vec, const auto &...args) -> bool
        {
            return ((std::find(vec.cbegin(), vec.cend(), args) != vec.cend()) || ...);
        };

    auto&& maze = mazes::runtime_app::instance();

    auto&& logger = mazes::global_async_logger();
    std::mutex mtx;
    std::vector<std::string> logs;
    logs.reserve(100);
    logger.set_sink([&logs, &mtx](auto msg)
        {
            std::lock_guard<std::mutex> lock(mtx);
            logs.emplace_back(msg); });

    // Copy command arguments and skip the program name
    const std::vector<std::string> args_vec{ argv + 1, argv + argc };

    static constexpr auto HELP_FLAG{ "-h" };
    static constexpr auto HELP_OPTION{ "--help" };
    static constexpr auto VERSION_FLAG{ "-v" };
    static constexpr auto VERSION_OPTION{ "--version" };

    if (args_vec.empty() || check_for_matches(std::cref(args_vec), HELP_FLAG, HELP_OPTION))
    {
        logger.log_message(parser->help());
    } else if (check_for_matches(std::cref(args_vec), VERSION_FLAG, VERSION_OPTION))
    {
        logger.log_message(parser->version());
    } else
    {
        std::string concatenated_args{};
        std::ranges::for_each(args_vec, [&concatenated_args](auto arg)
            { concatenated_args += arg + " "; });

        if (const auto results = maze->apply(concatenated_args); !results.empty())
        {
            logger.log_message(std::string{ results });
        }
    }

    // Ensure the async logger worker has delivered all queued messages
    // into the in-memory sink before we consume and print them.
    logger.flush();

    std::for_each(logs.cbegin(), logs.cend(), [](auto msg)
        { std::cout << msg << "\n"; });

    return EXIT_SUCCESS;
} // main
