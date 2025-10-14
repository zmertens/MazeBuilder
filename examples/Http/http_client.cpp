#include "http_client.h"

#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_map>

#include <SFML/Network.hpp>

http_client::http_client(const std::string& server_url)
    : m_server_url(server_url)
    , m_port(80)
{
    parse_server_url();
}

void http_client::parse_server_url() {
    // Parse URL to extract host and port
    std::regex url_regex(R"(^https?://([^:/]+)(?::(\d+))?(?:/.*)?$)");
    std::smatch matches;

    if (std::regex_match(m_server_url, matches, url_regex)) {
        m_host = matches[1].str();

        if (matches[2].matched) {
            m_port = static_cast<unsigned short>(std::stoi(matches[2].str()));
        } else {
            // Default ports
            if (m_server_url.find("https://") == 0) {
                m_port = 443;
            } else {
                m_port = 80;
            }
        }
    } else {
        // Fallback: treat the entire URL as host
        m_host = m_server_url;
        if (m_server_url.find("localhost") != std::string::npos ||
            m_server_url.find("127.0.0.1") != std::string::npos) {
            // Default for local development
            m_port = 3000;
        }
    }
}

std::string http_client::create_maze(int rows, int columns, int seed, const std::string& algorithm) {

    auto format_response = [](const sf::Http::Response& response) -> std::string {

        std::ostringstream oss;

        // Status line
        oss << "HTTP Response Status: " << static_cast<int>(response.getStatus());

        // Status name
        switch (response.getStatus()) {
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
        std::string body = response.getBody();
        if (!body.empty()) {
            oss << "Response Body:" << std::endl;
            oss << body;
        }

        return oss.str();
        };

    try {
        sf::Http http(m_host, m_port);

        std::string json_payload = create_json_payload(rows, columns, seed, algorithm);

        sf::Http::Request request("/api/mazes/create", sf::Http::Request::Method::Post);
        request.setHttpVersion(1, 1);
        request.setBody(json_payload);
        request.setField("Content-Type", "application/json");
        request.setField("Content-Length", std::to_string(json_payload.length()));

        sf::Http::Response response = http.sendRequest(request);
        return format_response(response);

    } catch (const std::exception& e) {
        return "Error creating maze: " + std::string(e.what());
    }
}

std::string http_client::create_json_payload(int rows, int columns, int seed, const std::string& algorithm) {
    std::ostringstream oss;
    oss << "{"
        << "\"rows\":" << rows << ","
        << "\"columns\":" << columns << ","
        << "\"levels\":1,"  // Default to 1 level for 2D mazes
        << "\"seed\":" << seed << ","
        << "\"algo\":\"" << algorithm << "\","
        << "\"str\":\"\""  // Empty string for maze representation
        << "}";
    return oss.str();
}
