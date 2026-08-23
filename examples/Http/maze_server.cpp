#include "maze_server.h"

#include <MazeBuilder/algos.h>
#include <MazeBuilder/bytes.h>
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/singleton_base.h>

#include <SFML/Network.hpp>

#include <any>
#include <array>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

const std::filesystem::path maze_server::MAZE_TEMP_IMAGE_PATH{std::filesystem::temp_directory_path() / "maze_server.png"};

maze_server::maze_server(unsigned short port)
    : m_port(port)
{
}

maze_server::~maze_server() = default;

void maze_server::stop() noexcept
{
    m_running = false;
}

void maze_server::run()
{
    sf::TcpListener listener;
    if (listener.listen(m_port) != sf::Socket::Status::Done)
    {
        throw std::runtime_error("Cannot bind to port " + std::to_string(m_port));
    }

    m_running = true;
    std::cout << "Maze server listening on http://localhost:" << m_port << "/mazes\n"
              << "  Try: curl \"http://localhost:" << m_port << "/mazes?rows=10&columns=8&algo=dfs\"\n"
              << "  Or just: curl \"http://localhost:" << m_port << "/mazes\"\n"
              << "Press Ctrl+C to stop.\n\n";

    while (m_running)
    {
        sf::TcpSocket client;
        if (listener.accept(client) != sf::Socket::Status::Done)
        {
            continue;
        }

        // A single recv is enough for HTTP GET (no request body)
        std::string buf(8192, '\0');
        std::size_t received = 0;
        if (client.receive(buf.data(), buf.size(), received) != sf::Socket::Status::Done)
        {
            continue;
        }

        buf.resize(received);
        const std::string response = handle_http_request(buf);
        if (std::any_of(response.cbegin(), response.cend(), [](char c) { return c != '\0'; }))
        {
            [[maybe_unused]]
            const auto status = client.send(response.data(), response.size());
        }

        // Log the request line
        const auto nl = buf.find('\n');
        std::cout << "[request] " << buf.substr(0, nl != std::string::npos ? nl : buf.size()) << '\n';
    }
}

// Parse "rows=10&columns=8&algo=dfs"
maze_server::query_params maze_server::parse_query_string(const std::string &qs)
{
    query_params p;
    std::istringstream stream(qs);
    std::string token;

    while (std::getline(stream, token, '&'))
    {
        const auto eq = token.find('=');
        if (eq == std::string::npos)
        {
            continue;
        }

        const std::string key = token.substr(0, eq);
        const std::string value = token.substr(eq + 1);

        try
        {
            if (key == "rows")
            {
                p.rows = static_cast<unsigned int>(std::stoi(value));
            }
            else if (key == "columns" || key == "cols")
            {
                p.columns = static_cast<unsigned int>(std::stoi(value));
            }
            else if (key == "algo")
            {
                p.algo = value;
            }
            p.use_distances = true; // default to true for now
        }
        catch (...)
        { /* keep defaults for malformed values */
        }
    }
    p.rows = std::clamp(p.rows, 1u, 100u);
    p.columns = std::clamp(p.columns, 1u, 100u);
    return p;
}

std::string maze_server::handle_http_request(const std::string &raw)
{
    // First line: "GET /mazes?rows=10&columns=8&algo=dfs HTTP/1.1"
    std::istringstream iss(raw);
    std::string method, path;
    if (!(iss >> method >> path))
    {
        return make_http_response(400, "Bad Request", "Malformed request line.");
    }

    if (method != "GET")
    {
        return make_http_response(405, "Method Not Allowed", "Only GET is supported.");
    }

    // Split path from query string
    std::string base = path;
    std::string query;
    if (const auto q = path.find('?'); q != std::string::npos)
    {
        base = path.substr(0, q);
        query = path.substr(q + 1);
    }
    // Strip trailing slash
    if (base.size() > 1 && base.back() == '/')
    {
        base.pop_back();
    }

    if (base != "/mazes")
    {
        return make_http_response(404, "Not Found",
                                  "404 Not Found\nEndpoint: GET /mazes?rows=N&columns=M&algo=X");
    }

    try
    {
        const auto p = parse_query_string(query);
        const auto body = generate_maze_text(p);
        return make_http_response(200, "OK", body);
    }
    catch (const std::exception &e)
    {
        return make_http_response(400, "Bad Request", std::string("Error: ") + e.what());
    }
}

std::string maze_server::generate_maze_text(const query_params &p)
{
    const auto app = mazes::singleton_base<mazes::runtime_app>::instance();
    if (!app)
    {
        throw std::runtime_error("Maze runtime is unavailable.");
    }

    mazes::randomizer rng{};
    const auto seed = rng(1u, 100u);

    // apply() dispatches to exactly one output state based on --output's extension:
    // PIXELIZING (png/bmp/jpg) returns a "Wrote maze to ..." confirmation, not the grid,
    // while STRINGIFYING (stdout/txt) returns the ASCII grid. Two calls (same seed) are
    // required to get both the grid text and the rendered image.
    std::ostringstream text_request;
    text_request << "--rows=" << p.rows
                 << " --columns=" << p.columns
                 << " --levels=1"
                 << " --algo=" << p.algo
                 << " --seed=" << seed
                 << " --distances=[0:-1]"
                 << " --output=stdout";

    // runtime_app::apply() returns a string_view into the app's internal buffer, which
    // gets overwritten by the next apply() call -- copy it into an owned string now,
    // before requesting the image, or "generated" would dangle/get corrupted below.
    const std::string generated{app->apply(text_request.str())};
    if (generated.empty())
    {
        throw std::runtime_error("Maze generation failed.");
    }

    std::ostringstream image_request;
    image_request << "--rows=" << p.rows
                  << " --columns=" << p.columns
                  << " --levels=1"
                  << " --algo=" << p.algo
                  << " --seed=" << seed
                  << " --distances=[0:-1]"
                  << " --output=" << MAZE_TEMP_IMAGE_PATH.string();

    if (app->apply(image_request.str()).empty())
    {
        throw std::runtime_error("Maze image generation failed.");
    }

    const auto image_bytes = mazes::io_utils::read_file_to_bytes(MAZE_TEMP_IMAGE_PATH);
    if (image_bytes.empty())
    {
        throw std::runtime_error("Maze image could not be read.");
    }

    const std::string image_base64 = mazes::bytes::encode(mazes::bytes::bytes_to_string(image_bytes));


    // First line: metadata; then the ASCII maze; then a delimiter followed by the
    // base64-encoded PNG so the response stays plain-text/binary-safe in one round trip.
    std::ostringstream oss;
    oss << "rows=" << p.rows
        << " columns=" << p.columns
        << " algo=" << p.algo
        << " use_distances=" << p.use_distances << "\n"
        << generated << maze_server::IMAGE_DELIMITER << image_base64;
    return oss.str();
}

std::string maze_server::make_http_response(int status_code,
                                            const std::string &status_text,
                                            const std::string &body)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n"
        << "Content-Type: text/plain\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << body;
    return oss.str();
}
