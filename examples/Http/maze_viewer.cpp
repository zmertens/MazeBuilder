#include "maze_viewer.h"

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    // Pixel dimensions for each ASCII character cell
    constexpr float CHAR_W = 10.f;
    constexpr float CHAR_H = 18.f;
    constexpr unsigned int STATUS_BAR_H = 42u;

    constexpr std::array<const char*, 3> ALGOS{"binary_tree", "sidewinder", "dfs"};

    // --- Font loading -------------------------------------------------------
    bool try_load_font(sf::Font& font)
    {
        for (const char* path : {
            "C:/Windows/Fonts/consola.ttf",
            "C:/Windows/Fonts/cour.ttf",
            "C:/Windows/Fonts/lucon.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
            "/System/Library/Fonts/Supplemental/Courier New.ttf",
        })
        {
            if (std::filesystem::exists(path) && font.openFromFile(path))
                return true;
        }
        return false;
    }

    // --- Maze rendering -----------------------------------------------------
    // Populate verts with two triangles per ASCII character.
    // Wall chars ('+', '-', '|') → dark slate; spaces → warm cream.
    void build_maze_vertices(const std::string& ascii,
                              sf::VertexArray&   verts,
                              float cw, float cell_h)
    {
        verts.clear();
        verts.setPrimitiveType(sf::PrimitiveType::Triangles);

        const sf::Color wall_col{50,  50,  70};
        const sf::Color pass_col{215, 205, 185};

        auto push_quad = [&](float x, float y, float w, float h, sf::Color c)
        {
            sf::Vertex tl, tr, bl, br;
            tl.position = {x,     y    };
            tr.position = {x + w, y    };
            bl.position = {x,     y + h};
            br.position = {x + w, y + h};
            tl.color = tr.color = bl.color = br.color = c;
            verts.append(tl); verts.append(tr); verts.append(bl);
            verts.append(tr); verts.append(br); verts.append(bl);
        };

        float y = 0.f;
        std::istringstream ss(ascii);
        std::string line;
        while (std::getline(ss, line))
        {
            float x = 0.f;
            for (const char tile : line)
            {
                push_quad(x, y, cw, cell_h, (tile == ' ') ? pass_col : wall_col);
                x += cw;
            }
            y += cell_h;
        }
    }

    // Measure ASCII dimensions (in characters)
    std::pair<unsigned, unsigned> ascii_dims(const std::string& ascii)
    {
        unsigned max_w = 0, h = 0;
        std::istringstream ss(ascii);
        std::string line;
        while (std::getline(ss, line))
        {
            max_w = std::max(max_w, static_cast<unsigned>(line.size()));
            ++h;
        }
        return {max_w, h};
    }

} // anonymous namespace

// ---------------------------------------------------------------------------

maze_viewer::maze_viewer(std::string host, unsigned short port)
    : m_host(std::move(host)), m_port(port)
{}

std::string maze_viewer::fetch_ascii(unsigned int rows, unsigned int columns,
                                     const std::string& algo)
{
    sf::Http http(m_host, m_port);

    const std::string uri = "/mazes?rows="    + std::to_string(rows)
                          + "&columns="       + std::to_string(columns)
                          + "&algo="          + algo;

    sf::Http::Request req(uri, sf::Http::Request::Method::Get);
    req.setHttpVersion(1, 1);
    req.setField("Connection", "close");

    const auto resp = http.sendRequest(req, sf::seconds(5.f));

    if (resp.getStatus() != sf::Http::Response::Status::Ok)
        throw std::runtime_error("HTTP " +
            std::to_string(static_cast<int>(resp.getStatus())));

    return resp.getBody();
}

int maze_viewer::run()
{
    sf::Font font;
    if (!try_load_font(font))
    {
        std::cerr << "maze_viewer: no usable monospace font found.\n";
        return EXIT_FAILURE;
    }

    // ---- Helpers -----------------------------------------------------------
    sf::VertexArray verts(sf::PrimitiveType::Triangles);

    auto apply_fetch = [&](bool rebuild_window, sf::RenderWindow& window)
    {
        try
        {
            const auto body = fetch_ascii(m_rows, m_columns, ALGOS[m_algo_idx]);
            const auto nl   = body.find('\n');
            m_ascii  = (nl != std::string::npos) ? body.substr(nl + 1) : body;
            m_status = (nl != std::string::npos) ? body.substr(0, nl) : "";
            build_maze_vertices(m_ascii, verts, CHAR_W, CHAR_H);

            if (rebuild_window)
            {
                const auto [cw, ch] = ascii_dims(m_ascii);
                const auto win_w = std::max(static_cast<unsigned>(cw * CHAR_W), 400u);
                const auto win_h = std::max(static_cast<unsigned>(ch * CHAR_H) + STATUS_BAR_H, 300u);
                window.setSize({win_w, win_h});
                window.setView(sf::View(sf::FloatRect({0.f, 0.f},
                    {static_cast<float>(win_w), static_cast<float>(win_h)})));
            }

            window.setTitle(std::string("MazeBuilder – ")
                          + ALGOS[m_algo_idx] + "  "
                          + m_host + ":" + std::to_string(m_port));
        }
        catch (const std::exception& e)
        {
            m_status = std::string("Server error: ") + e.what()
                     + "  (is the server running?)";
        }
    };

    // ---- Initial window ----------------------------------------------------
    sf::RenderWindow window(
        sf::VideoMode({600u, 400u}),
        "MazeBuilder HTTP Client");
    window.setFramerateLimit(30u);

    // Do an initial fetch to size the window properly
    apply_fetch(true, window);

    // ---- Overlay text ------------------------------------------------------
    const std::string help_str =
        "G/Enter: generate   Tab: algo   +/-: size   Q/Esc: quit";

    sf::Text help_text(font, help_str, 13u);
    help_text.setFillColor(sf::Color(220, 220, 220));
    help_text.setOutlineColor(sf::Color::Black);
    help_text.setOutlineThickness(1.f);
    help_text.setPosition({6.f, 4.f});

    sf::Text status_text(font, "", 13u);
    status_text.setFillColor(sf::Color(140, 210, 140));
    status_text.setOutlineColor(sf::Color::Black);
    status_text.setOutlineThickness(1.f);
    status_text.setPosition({6.f, 22.f});

    // ---- Event loop --------------------------------------------------------
    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto* kp = event->getIf<sf::Event::KeyPressed>())
            {
                switch (kp->code)
                {
                case sf::Keyboard::Key::Escape:
                case sf::Keyboard::Key::Q:
                    window.close();
                    break;

                case sf::Keyboard::Key::G:
                case sf::Keyboard::Key::Enter:
                    apply_fetch(true, window);
                    break;

                case sf::Keyboard::Key::Tab:
                    m_algo_idx = (m_algo_idx + 1) % ALGOS.size();
                    apply_fetch(true, window);
                    break;

                case sf::Keyboard::Key::Add:
                case sf::Keyboard::Key::Equal:
                    m_rows    = std::min(m_rows    + 2u, 40u);
                    m_columns = std::min(m_columns + 2u, 40u);
                    apply_fetch(true, window);
                    break;

                case sf::Keyboard::Key::Subtract:
                case sf::Keyboard::Key::Hyphen:
                    m_rows    = std::max(m_rows    - 2u, 4u);
                    m_columns = std::max(m_columns - 2u, 4u);
                    apply_fetch(true, window);
                    break;

                default:
                    break;
                }
            }
        }

        // Update status text each frame
        status_text.setString(m_status);

        window.clear(sf::Color(30, 30, 40));

        // Status bar background
        sf::RectangleShape bar;
        bar.setSize({static_cast<float>(window.getSize().x),
                     static_cast<float>(STATUS_BAR_H)});
        bar.setFillColor(sf::Color(18, 18, 28, 230));
        bar.setPosition({0.f, 0.f});
        window.draw(bar);

        window.draw(help_text);
        window.draw(status_text);

        // Draw maze below status bar
        sf::RenderStates states;
        states.transform.translate({0.f, static_cast<float>(STATUS_BAR_H)});
        window.draw(verts, states);

        window.display();
    }

    return EXIT_SUCCESS;
}
