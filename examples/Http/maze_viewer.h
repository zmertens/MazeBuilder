#ifndef MAZE_VIEWER_H
#define MAZE_VIEWER_H

#include <string>

/// @brief SFML window client that fetches mazes from the maze_server and
///        renders them visually as a 2-D grid.
///
/// Keyboard controls:
///   G / Enter  – fetch a new maze from the server
///   Tab        – cycle algorithm  (binary_tree → sidewinder → dfs)
///   + / =      – grow maze by 2 rows & columns
///   - / _      – shrink maze by 2 rows & columns
///   Q / Esc    – quit
class maze_viewer
{
public:
    maze_viewer(std::string host, unsigned short port);

    /// @brief Open the SFML window and run the event loop.
    /// @return EXIT_SUCCESS or EXIT_FAILURE.
    int run();

private:
    std::string fetch_ascii(unsigned int rows, unsigned int columns,
                            const std::string& algo);

    std::string m_host;
    unsigned short m_port;

    unsigned int m_rows{10};
    unsigned int m_columns{10};
    std::size_t  m_algo_idx{0};   // index into ALGOS array

    std::string m_ascii;          // current maze text (no metadata line)
    std::string m_status;         // status bar line
};

#endif // MAZE_VIEWER_H
