// Send HTTP Requests to create mazes

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "terminal_gui.h"

static constexpr auto USAGE_MSG = R"(
Maze Builder HTTP Client - Terminal Interface - v0.1.0

Usage:
  maze_client <server_url>

Arguments:
  server_url    URL of the Corners server
                Examples:
                  http://localhost:3000 (for development)

Description:
  This application provides a terminal interface for interacting with the Corners
  maze building server. Once started, you can use various commands to create mazes.

  Available terminal commands:
    maze_client --help                     Show maze builder help
    maze_client --create                   Create a new maze
    ls                                     List available programs
    find <pattern>                         Find programs matching pattern
    help                                   Show terminal help
    exit                                   Exit the application
)";

void print_usage() {

    std::cout << USAGE_MSG << std::endl;
}

bool is_valid_url(const std::string& url) {

    // Basic URL validation - SFML Network doesn't support HTTPS
    return url.find("http://") == 0 && url.find("https://") != 0;
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    using namespace std;

    static constexpr auto TOTAL_ARG_COUNT = 2;

    // Check command line arguments
    if (argc != TOTAL_ARG_COUNT) {

        std::cerr << "Error: Invalid number of arguments." << std::endl;

        print_usage();

        return EXIT_FAILURE;
    }

    std::string server_url = argv[1];

    // Handle help request
    if (server_url == "--help" || server_url == "-h") {

        print_usage();

        return EXIT_SUCCESS;
    }

    // Validate URL
    if (!is_valid_url(server_url)) {

        std::cerr << "Error: Invalid server URL. Must start with http:// (https:// is not supported)." << std::endl;

        print_usage();

        return EXIT_FAILURE;
    }

    try {

        // Create and initialize terminal GUI
        terminal_gui gui{};

        gui.initialize(server_url);

        // Start the terminal interface
        gui.run();

    } catch (const std::exception& e) {

        std::cerr << "Error: " << e.what() << std::endl;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}



