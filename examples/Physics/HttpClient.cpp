#include "HttpClient.hpp"

#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_map>

#include <SFML/Network.hpp>

HttpClient::HttpClient(const std::string& server_url)
    : m_server_url(server_url)
      , m_port(80)
{
    parse_server_url();
}

void HttpClient::parse_server_url()
{
    using std::regex;
    using std::smatch;
    using std::string;
    using std::stoi;
    using std::ostringstream;
    using std::cout;
    using std::endl;

    // Parse URL to extract host and port
    regex url_regex(R"(^https?://([^:/]+)(?::(\d+))?(?:/.*)?$)");
    smatch matches;

#if defined(MAZE_DEBUG)

    cout << "DEBUG: Parsing URL: " << m_server_url << endl;
#endif

    if (regex_match(m_server_url, matches, url_regex))
    {
        m_host = matches[1].str();

#if defined(MAZE_DEBUG)

        cout << "DEBUG: Regex matched. Host: " << m_host << endl;
#endif

        if (matches[2].matched)
        {
            m_port = static_cast<unsigned short>(stoi(matches[2].str()));

#if defined(MAZE_DEBUG)

            cout << "DEBUG: Port from URL: " << m_port << endl;
#endif
        }
        else
        {
            // Default ports
            if (m_server_url.find("https://") == 0)
            {
                m_port = 443;
            }
            else
            {
                m_port = 80;
            }

#if defined(MAZE_DEBUG)

            cout << "DEBUG: Using default port: " << m_port << endl;
#endif
        }
    }
    else
    {
#if defined(MAZE_DEBUG)

        cout << "DEBUG: Regex failed to match URL" << endl;
#endif

        // Fallback: treat the entire URL as host
        m_host = m_server_url;
        if (m_server_url.find("localhost") != string::npos ||
            m_server_url.find("127.0.0.1") != string::npos)
        {
            // Default for local development
            m_port = 3000;
        }
#if defined(MAZE_DEBUG)

        cout << "DEBUG: Fallback - Host: " << m_host << ", Port: " << m_port << endl;
#endif
    }

#if defined(MAZE_DEBUG)

    cout << "DEBUG: Final parsed - Host: " << m_host << ", Port: " << m_port << endl;
#endif
}

std::string HttpClient::create_maze(int rows, int columns, int seed, const std::string& algorithm,
                                    const std::string& distances)
{
    auto format_response = [](const sf::Http::Response& response) -> std::string
    {
        std::ostringstream oss;

        // Status line
        oss << "HTTP Response Status: " << static_cast<int>(response.getStatus());

        // Status name
        switch (response.getStatus())
        {
        case sf::Http::Response::Status::Ok:
            oss << " (OK)";
            break;
        case sf::Http::Response::Status::Created:
            oss << " (Created)";
            break;
        case sf::Http::Response::Status::BadRequest:
            oss << " (Bad Request)";
            break;
        case sf::Http::Response::Status::NotFound:
            oss << " (Not Found)";
            break;
        case sf::Http::Response::Status::InternalServerError:
            oss << " (Internal Server Error)";
            break;
        default:
            break;
        }

        oss << std::endl;

        // Response body
        if (auto body{response.getBody()}; !body.empty())
        {
            oss << "Response Body:" << std::endl;

            oss << body;
        }

        return oss.str();
    };

    try
    {
#if defined(MAZE_DEBUG)

        std::cout << "DEBUG: Connecting to " << m_host << ":" << m_port << std::endl;
#endif

        sf::Http http(m_host, m_port);

        std::string json_payload = create_json_payload(rows, columns, seed, algorithm, distances);
#if defined(MAZE_DEBUG)

        std::cout << "DEBUG: JSON payload: " << json_payload << std::endl;
#endif

        sf::Http::Request request("/api/mazes/create", sf::Http::Request::Method::Post);
        request.setHttpVersion(1, 1);
        request.setBody(json_payload);
        request.setField("Content-Type", "application/json");
        request.setField("Content-Length", std::to_string(json_payload.length()));

        sf::Http::Response response = http.sendRequest(request);
        return format_response(response);
    }
    catch (const std::exception& e)
    {
        return "Error creating maze: " + std::string(e.what());
    }
}

std::string HttpClient::create_json_payload(int rows, int columns, int seed, const std::string& algorithm,
                                            const std::string& distances)
{
    std::ostringstream oss;
    oss << "{"
        << "\"rows\":" << rows << ","
        << "\"columns\":" << columns << ","
        << "\"levels\":1," // Default to 1 level for 2D mazes
        << "\"seed\":" << seed << ","
        << "\"algo\":\"" << algorithm << "\","
        << "\"distances\":\"" << distances << "\""
        << "}";

    return oss.str();
}
