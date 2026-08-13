#ifndef MAZE_SERVER_H
#define MAZE_SERVER_H

#include <filesystem>
#include <string>
#include <string_view>

/// @brief Minimal single-threaded HTTP server that serves generated mazes.
///
/// Handles GET /mazes with optional query parameters:
///   rows     – number of rows    (default 10, clamped to [1, 100])
///   columns  – number of columns (default 10, clamped to [1, 100])
///   algo     – binary_tree | sidewinder | dfs  (default binary_tree)
///
/// Response body (plain-text, binary-safe): a metadata line, then the ASCII
/// maze, then IMAGE_DELIMITER followed by the maze PNG encoded as base64.
/// Example URL: http://localhost:8080/mazes?rows=12&columns=10&algo=dfs
class maze_server
{
public:
    explicit maze_server(unsigned short port = 8080);
    ~maze_server();

    maze_server(const maze_server &) = delete;
    maze_server &operator=(const maze_server &) = delete;

    /// @brief Block and serve requests until stop() is called or an error occurs.
    void run();

    /// @brief Signal the server loop to exit after the current request finishes.
    void stop() noexcept;

    /// @brief Marks the end of the ASCII maze and the start of the base64-encoded PNG.
    static constexpr std::string_view IMAGE_DELIMITER{"\n--MAZE-IMAGE-BASE64--\n"};

private:
    static const std::filesystem::path MAZE_TEMP_IMAGE_PATH;

    struct query_params
    {
        bool use_distances{true};
        unsigned int rows{10};
        unsigned int columns{10};
        std::string algo{"binary_tree"};
    };

    static query_params parse_query_string(const std::string &qs);
    static std::string handle_http_request(const std::string &raw);
    static std::string generate_maze_text(const query_params &p);

    static std::string make_http_response(int status_code,
                                          const std::string &status_text,
                                          const std::string &body);

    unsigned short m_port;
    bool m_running{false};
};

#endif // MAZE_SERVER_H
