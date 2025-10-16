#include "terminal_gui.h"

#include "http_client.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

void terminal_gui::initialize(const std::string& server_url) {

    m_server_url = server_url;

    m_current_directory = "http";

    m_available_programs = { "find", "mazebuilderhttp", "ls", "help", "exit" };

    m_running = true;

    std::cout << "Terminal initialized with Corners server: " << m_server_url << std::endl;

    std::cout << "Type 'help' for available commands or 'mazebuilderhttp --help' for maze builder options." << std::endl;

    std::cout << std::endl;

    register_commands();
}

void terminal_gui::run() {

    if (!m_running) {

        std::cerr << "Terminal not initialized. Call initialize() first." << std::endl;

        return;
    }

    std::string input{};

    while (m_running) {

        display_prompt();

        if (!std::getline(std::cin, input)) {

            // EOF or error
            break;
        }

        // Trim whitespace
        input.erase(0, input.find_first_not_of(" \t"));
        input.erase(input.find_last_not_of(" \t") + 1);

        if (input.empty()) {

            continue;
        }

        std::string output = process_command(input);

        if (!output.empty()) {

            std::cout << output << std::endl;
        }
    }
}

std::string terminal_gui::process_command(const std::string& command) {

    auto args = parse_arguments(command);

    if (args.empty()) {

        return "";
    }

    std::string cmd = args[0];

    // Find the command handler
    auto it = m_commands.find(cmd);

    if (it != m_commands.end()) {

        return it->second(args);
    } else {

        return "Command not found: " + cmd;
    }
}

void terminal_gui::display_prompt() const {

    std::cout << "builder123@mazes:~/" << m_current_directory << "$ ";
}

void terminal_gui::register_commands() {

    m_commands["mazebuilderhttp"] = [this](const std::vector<std::string>& args) {

        return handle_mazebuilderhttp(args);
        };

    m_commands["ls"] = [this](const std::vector<std::string>& args) {

        return handle_ls(args);
        };

    m_commands["find"] = [this](const std::vector<std::string>& args) {

        return handle_find(args);
        };

    m_commands["help"] = [this](const std::vector<std::string>& args) {

        return handle_help(args);
        };

    m_commands["exit"] = [this](const std::vector<std::string>& args) {

        return handle_exit(args);
        };
}

std::vector<std::string> terminal_gui::parse_arguments(const std::string& command_line) const {

    std::vector<std::string> args;

    std::istringstream iss(command_line);

    std::string arg;

    while (iss >> std::quoted(arg)) {

        args.push_back(arg);
    }

    return args;
}

std::string terminal_gui::handle_mazebuilderhttp(const std::vector<std::string>& args) {

    if (args.size() < 2) {

        return show_mazebuilder_help();
    }

    std::string subcommand = args[1];

    if (subcommand == "--help" || subcommand == "-h") {

        return show_mazebuilder_help();
    } else if (subcommand == "--create") {

        // Parse create command arguments
        auto rows{ 10 }, columns{ 10 }, seed{ 42 };

        std::string algorithm{ "dfs" };

        for (size_t i = 2; i < args.size(); i++) {

            if (args[i] == "-r" && i + 1 < args.size()) {

                rows = std::stoi(args[++i]);
            } else if (args[i] == "-c" && i + 1 < args.size()) {

                columns = std::stoi(args[++i]);
            } else if (args[i] == "-s" && i + 1 < args.size()) {

                seed = std::stoi(args[++i]);
            } else if (args[i] == "-a" && i + 1 < args.size()) {

                algorithm = args[++i];
            }
        }

        return create_maze(rows, columns, seed, algorithm);
    } else {

        return "Unknown mazebuilderhttp command: " + subcommand + "\nUse 'mazebuilderhttp --help' for usage information.";
    }
}

std::string terminal_gui::handle_ls(const std::vector<std::string>& args) {

    std::ostringstream oss;

    for (const auto& program : m_available_programs) {

        oss << program << "  ";
    }

    return oss.str();
}

std::string terminal_gui::handle_find(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return "Usage: find <pattern>";
    }

    std::string pattern = args[1];
    std::ostringstream oss;

    for (const auto& program : m_available_programs) {
        if (program.find(pattern) != std::string::npos) {
            oss << program << std::endl;
        }
    }

    std::string result = oss.str();
    if (result.empty()) {
        return "No programs found matching pattern: " + pattern;
    }

    return result;
}

std::string terminal_gui::handle_help(const std::vector<std::string>& args) {
    return R"(Available commands:
  mazebuilderhttp  - HTTP client for Corners maze building server
  ls              - List available programs
  find <pattern>  - Find programs matching pattern
  help            - Show this help message
  exit            - Exit the terminal

Use 'mazebuilderhttp --help' for detailed maze builder options.)";
}

std::string terminal_gui::handle_exit(const std::vector<std::string>& args) {
    m_running = false;
    return "Goodbye!";
}

std::string terminal_gui::create_maze(int rows, int columns, int seed, const std::string& algorithm) {
    http_client client(m_server_url);
    return client.create_maze(rows, columns, seed, algorithm);
}

std::string terminal_gui::show_mazebuilder_help() const {
    return R"(mazebuilderhttp - HTTP client for Corners maze building server

Usage:
  mazebuilderhttp --help                     Show this help message
  mazebuilderhttp --create -r <rows> -c <columns> -s <seed> -a <algorithm>
                                            Create a new maze
  mazebuilderhttp --list                    Get all mazes from server
  mazebuilderhttp --delete <id>             Delete maze by ID

Options:
  -r, --rows <number>      Number of rows (default: 10)
  -c, --columns <number>   Number of columns (default: 10)
  -s, --seed <number>      Random seed (default: 42)
  -a, --algorithm <name>   Algorithm to use (default: dfs)
                          Available: dfs, binary_tree, sidewinder

Examples:
  mazebuilderhttp --create -r 10 -c 10 -s 42 -a dfs)";
}
