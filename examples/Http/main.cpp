/// @file main.cpp
/// @brief MazeBuilder HTTP example -- single executable, two modes.
///
/// Usage:
///   mazebuilderhttp --server [--port <N>]
///   mazebuilderhttp --client [--host <host>] [--port <N>]

#include "maze_server.h"
#include "maze_viewer.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

static void print_usage(const char* prog)
{
    std::cout <<
"MazeBuilder HTTP Example -- local server + SFML client\n\n"
"Usage:\n"
"  <prog> --server [--port <N>]\n"
"  <prog> --client [--host <host>] [--port <N>]\n\n"
"Server mode\n"
"  Listens for HTTP GET and returns generated mazes as plain text.\n"
"  Default port: 8080\n\n"
"Client mode\n"
"  Opens an SFML window, queries the server, renders the maze visually.\n"
"  Default: localhost:8080\n\n"
"Server endpoint\n"
"  GET /mazes\n"
"  GET /mazes?rows=12&columns=10&algo=dfs\n\n"
"Query parameters\n"
"  rows     number of rows    (default 10, clamped 1-100)\n"
"  columns  number of columns (default 10, clamped 1-100)\n"
"  algo     binary_tree | sidewinder | dfs  (default binary_tree)\n\n"
"Client keyboard controls\n"
"  G / Enter   fetch a new maze\n"
"  Tab         cycle algorithms\n"
"  + / =       grow maze (+2 per dim)\n"
"  - / _       shrink maze (-2 per dim)\n"
"  Q / Esc     quit\n\n"
"Examples\n"
"  mazebuilderhttp --server\n"
"  mazebuilderhttp --client\n"
"  curl \"http://localhost:8080/mazes?rows=15&columns=12&algo=dfs\"\n\n";
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const std::string mode = argv[1];
    if (mode == "--help" || mode == "-h")
    {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    unsigned short port = 8080;
    std::string    host = "localhost";

    for (int i = 2; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if ((arg == "--port" || arg == "-p") && i + 1 < argc)
            port = static_cast<unsigned short>(std::stoi(argv[++i]));
        else if ((arg == "--host" || arg == "-H") && i + 1 < argc)
            host = argv[++i];
    }

    if (mode == "--server")
    {
        try
        {
            maze_server server(port);
            server.run();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Server error: " << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }
    else if (mode == "--client")
    {
        maze_viewer viewer(host, port);
        return viewer.run();
    }
    else
    {
        std::cerr << "Unknown mode: " << mode << '\n';
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
