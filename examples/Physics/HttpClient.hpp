#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include <string>

/// @brief HTTP client for communicating with Corners maze building server
class HttpClient
{
public:
    /// @brief Constructor
    /// @param server_url Base URL of the Corners server
    explicit HttpClient(const std::string& server_url);

    /// @brief Create a new maze via HTTP POST request
    /// @param rows Number of rows
    /// @param columns Number of columns
    /// @param seed Random seed
    /// @param algorithm Algorithm to use
    /// @return JSON response from server
    std::string create_maze(int rows, int columns, int seed, const std::string& algorithm,
                            const std::string& distances = {"[0:-1]"});

private:
    /// @brief Parse server URL and extract host and port
    void parse_server_url();

    /// @brief Create JSON payload for maze creation
    /// @param rows Number of rows
    /// @param columns Number of columns
    /// @param seed Random seed
    /// @param algorithm Algorithm to use
    /// @return JSON string
    std::string create_json_payload(int rows, int columns, int seed, const std::string& algorithm,
                                    const std::string& distances);

    std::string m_server_url;
    std::string m_host;
    unsigned short m_port;
};

#endif // HTTP_CLIENT_HPP
