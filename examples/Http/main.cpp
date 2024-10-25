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

#include <nlohmann/json.hpp>

#include <MazeBuilder/maze_builder.h>

static constexpr auto MB_MENU_MSG = R"help(
    -- Maze Builder Menu --
    1. Create New Maze
    2. Get a Maze from the Network
    3. Print Menu Options
    4. Show Example
    5. Exit
    [version: 0.1.0]
    )help";

static constexpr auto WORKER_URL = "http://mb-worker.zach-mm035.workers.dev";
static constexpr auto LOCAL_URL = "http://127.0.0.1";

void print_menu() {
    std::cout << MB_MENU_MSG;
#if defined(MAZE_DEBUG)
    std::cout << "\nHttp URL: " << LOCAL_URL << std::endl;
#else
    std::cout << "\nHttp URL: " << WORKER_URL << std::endl;
#endif
}

void process_commands(std::deque<char>& commands,
    bool& is_running,
    const std::unique_ptr<mazes::maze_builder::maze>& mb) {
    
    using namespace std;

    mt19937 rng;
    auto get_int = [&rng](auto low, auto high) {
        std::uniform_int_distribution<int> dist(low, high);
        return dist(rng);
    };

    auto process_char = [&mb, &is_running, &get_int, &rng](char ch) {
        sf::Http::Request sf_post_request {"api/mazes", sf::Http::Request::Post};
        sf_post_request.setHttpVersion(1, 1);
        sf::Http::Request sf_get_request {"api/mazes", sf::Http::Request::Get};
        sf_get_request.setHttpVersion(1, 1);
#if defined(MAZE_DEBUG)
        sf::Http http{ LOCAL_URL, 8787 };
#else
        sf::Http http{ WORKER_URL };
#endif
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
            // Create the maze
            mb->rows = rows;
            mb->columns = columns;
            mb->height = depth;
            rng.seed(seed);
            mb->rng = rng;
            auto dump = mb->to_json_str(4);
            sf_post_request.setBody(dump);
            sf_post_request.setField("Content-Type", "application/json");
            sf_post_request.setField("Content-Length", to_string(dump.size()));

            // Note the base64 encoded string is very long
            //cout << "Sent new maze:\n" << my_json.dump(4) << endl;

            response = http.sendRequest(sf_post_request);
            cout << "Response status: " << response.getStatus() << " \nbody: " << response.getBody() << endl;
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
            print_menu();
            break;
            case '4': {
            // Show example
            cout << "Showing example maze builder...\n";
            ostringstream option_1;
            option_1 << "-- Enter rows, columns, depth, seed, and algorithm with a single space between. --\n";
            ostringstream example_user_input;
            example_user_input << "Example (ignore [] brackets):\t[1 2 3 1 dfs]\n";
            
            cout << option_1.str() << example_user_input.str();
            
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
    }
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{
    using namespace std;

    mazes::maze_builder builder;
    auto my_maze = builder.block_type(-1).rows(10).columns(10).height(1).seed(1).maze_type(mazes::maze_types::DFS).build();

    float accumulator = 0.0;
    static constexpr auto FIXED_TIMESTEP = 1.0f / 60.0f;

    sf::Clock clock;

    print_menu();

    deque<char> commands;
    bool is_running = true;
    // Main game loop
    while (is_running) {
        // Input / Update
        cin.clear();
        if (auto ch = cin.get(); ch != '\n' && ch != EOF) {
            commands.push_back(ch);
        }

        // Process commands
        process_commands(commands, ref(is_running), cref(my_maze));
    }

    return 0;
} // main



