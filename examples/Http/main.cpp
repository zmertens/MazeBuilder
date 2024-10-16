/**
 * Makes basic HTTP request to a server using SFML
 * POSTS/GETs data to/from server
 */

#include <memory>
#include <vector>
#include <variant>
#include <stack>
#include <iostream>
#include <random>
#include <functional>

#include <SFML/Network.hpp>

#include <MazeBuilder/maze_builder.h>

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{
    using namespace std;

    mt19937 rng;
    auto get_int = [&rng](auto low, auto high) {
        std::uniform_int_distribution<int> dist(low, high);
        return dist(rng);
    };
    rng.seed(100);
    
    // SFML HTTP Request
    string url {"hello-wrangler.zach-mm035.workers.dev"};
    sf::Http sf_request(url);
    sf::Http::Request request;
    request.setMethod(sf::Http::Request::Get);
    // request.setUri("/upload");
    request.setHttpVersion(1, 1);
    request.setField("Content-Type", "plain/txt");
    // request.setBody(std::string("Hello from the desktop!"));

    sf::Http::Response response = sf_request.sendRequest(request);

    if (response.getStatus() == sf::Http::Response::Ok) {
        std::cout << "Response received:\n" << response.getBody() << std::endl;
    } else {
        std::cout << "Request failed with status code: " << response.getStatus() << std::endl;
    }

    // MazeBuilder<BinaryTree> builder2;
    // auto bt = builder2.x(2).y(3).z(4).build();
    // cout << bt << endl;

    // MyVariant my_variant = bt;
    // visit([](auto&& arg) { cout << arg << endl; }, my_variant);

    double accumulator = 0.0;
    static constexpr auto FIXED_TIMESTEP = 1.0 / 60.0;

    // 5000 ms to query from the DB
    static constexpr auto REQUEST_INTERVAL = 5000;

    sf::Clock clock;
    unsigned int counter {0};
    // Main game loop
    while (true) {
        // Input / Update
        accumulator += clock.restart().asSeconds();
        while (accumulator >= FIXED_TIMESTEP) {
            accumulator -= FIXED_TIMESTEP;
            counter += 1;
        }

        if (counter % 3000 == 0) {
            counter = 0;
        }
    }
} // main



