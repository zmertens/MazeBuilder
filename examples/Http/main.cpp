/**
 * Makes basic HTTP request to a server using SFML
 * POSTS/GETs data to/from server:
 *  On user actions - get top 10 scores - send new score
 */

#include <memory>
#include <vector>
#include <variant>
#include <sstream>
#include <iostream>
#include <random>
#include <functional>
#include <deque>

#include <SFML/Network.hpp>

#include <MazeBuilder/maze_builder.h>

static std::shared_ptr<mazes::maze_builder> my_maze_builder = std::make_shared<mazes::maze_builder>(1, 1, 1);

static constexpr auto MB_MENU_MSG = R"help(
    -- Maze Builder Menu --
    1. Create New Maze
    2. Get a Maze from the Network
    3. Print Menu Options
    4. Show Example
    5. Exit
    [version: 0.1.0]
    )help";

static constexpr auto WORKER_URL = "hello-wrangler.zach-mm035.workers.dev";

void print_menu() {
    std::cout << MB_MENU_MSG;
    std::cout << "\nHttp URL: " << WORKER_URL << std::endl;
}

void process_commands(std::deque<char>& commands,
    bool& is_running,
    const std::shared_ptr<mazes::maze_builder>& mb) {
    
    using namespace std;

    mt19937 rng;
    auto get_int = [&rng](auto low, auto high) {
        std::uniform_int_distribution<int> dist(low, high);
        return dist(rng);
    };

    auto process_char = [&mb, &is_running, &get_int, &rng](char ch) {
        sf::Http::Request sf_post_request {WORKER_URL, sf::Http::Request::Post};
        sf_post_request.setHttpVersion(1, 1);
        sf::Http::Request sf_get_request {WORKER_URL, sf::Http::Request::Get};
        sf_get_request.setHttpVersion(1, 1);
        sf::Http http{ WORKER_URL };
        sf::Http::Response response;

        string users_maze_output{};

        switch (ch) {
            case '1': {
            cout << "Creating new maze...\n";
            cout << "Enter rows, columns, depth, seed, and algorithm with a single space between: ";
            int rows, columns, depth, seed;
            string algorithm;
            // Get user input
            cin >> rows >> columns >> depth >> seed >> algorithm;
            mazes::maze_types mt = mazes::maze_types::DFS;
            if (algorithm.compare("dfs") == 0) {
                mt = mazes::maze_types::DFS;
            } else if (algorithm.compare("sidewinder") == 0) {
                mt = mazes::maze_types::SIDEWINDER;
            } else if (algorithm.compare("binary_tree") == 0) {
                mt = mazes::maze_types::BINARY_TREE;
            } else {
                cerr << "Invalid algorithm. Please try again.\n";
                break;
            }
            my_maze_builder->clear();
            my_maze_builder = std::make_shared<mazes::maze_builder>(rows, columns, depth);
            rng.seed(seed);
            users_maze_output = my_maze_builder->to_str(mt, cref(get_int), cref(rng), true);
            // @TODO send URL params or JSON body
            sf_post_request.setBody(users_maze_output);
            sf_post_request.setField("Content-Type", "text/plain");
            sf_post_request.setField("Content-Length", to_string(users_maze_output.size()));

            cout << "Sent new maze:{\n\trows=" << rows << ";columns=" << columns << ";depth=" << depth << ";seed=" << seed << ";algorithm='" << algorithm << "';str=\n'" << users_maze_output << "';\n}\n";

            response = http.sendRequest(sf_post_request);
            if (response.getStatus() != sf::Http::Response::Ok) {
                cerr << "Error: " << response.getStatus() << endl;
                break;
            }
            cout << "Response from server: " << response.getBody() << endl;
            break;
            } // case '1'
            case '2': {
            cout << "Getting maze from the network...\n";
            response = http.sendRequest(sf_get_request);
            if (response.getStatus() != sf::Http::Response::Ok) {
                cerr << "Error: " << response.getStatus() << endl;
                break;
            }
            cout << "Response from server: " << response.getBody() << endl;
            break;
            } // case '2'
            case '3':
            cout << MB_MENU_MSG << endl;
            break;
            case '4': {
            // Show example
            cout << "Showing example maze builder...\n";
            ostringstream option_1;
            option_1 << "-- Enter rows, columns, depth, seed, and algorithm with a single space between. --\n";
            ostringstream example_user_input;
            example_user_input << "Example (ignore [] brackets):\t[1 2 3 1 dfs]\nSent new maze using sf::Http:\n{\trows=1;\n\tcolumns=2;\n\tdepth=3;\n\tseed=1;\n\talgorithm='dfs';}\n";
            ostringstream example_1;
            example_1 << option_1.str() << example_user_input.str();
            cout << example_1.str();
            break;
            }
            case '5':
            cout << "Exiting Maze Builder...\n";
            is_running = false;
            break;
            default:
            cout << "Invalid option. Please try again.\n";
            break;
        }
    };

    while (!commands.empty()) {
        if (auto ch = commands.front()) {
            commands.pop_front();
            process_char(ch);
        }
        //commands.clear();
        if (is_running) {
            print_menu();
        }
    }
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{
    using namespace std;

    float accumulator = 0.0;
    static constexpr auto FIXED_TIMESTEP = 1.0f / 60.0f;

    sf::Clock clock;

    print_menu();

    deque<char> commands;
    bool is_running = true;
    // Main game loop
    while (is_running) {
        // Input / Update
        accumulator += clock.restart().asSeconds();
        while (accumulator >= FIXED_TIMESTEP) {
            cin.clear();
            if (auto ch = cin.get(); ch != '\n' && ch != EOF) {
                commands.push_back(ch);
            }
            accumulator -= FIXED_TIMESTEP;

            // Process commands
            process_commands(commands, ref(is_running), cref(my_maze_builder));
        }
        accumulator = min(accumulator, FIXED_TIMESTEP);
    }
} // main



