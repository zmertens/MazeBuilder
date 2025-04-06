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

static constexpr auto MB_MENU_MSG = R"help(
    -- Maze Builder Menu --
    1. Create a new maze
    2. Get all mazes from the DB
    3. Delete a maze by id
    4. Print Menu Options
    5. Show Example
    6. Exit
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
    std::cout << "version: " << mazes::build_info::Version + "-" + mazes::build_info::CommitSHA << std::endl << std::endl;
}

void process_commands(std::deque<char>& commands, bool& is_running) {
    
    using namespace std;

    mt19937 rng;
    auto get_int = [&rng](auto low, auto high) {
        std::uniform_int_distribution<int> dist(low, high);
        return dist(rng);
    };

    auto process_char = [&is_running, &get_int, &rng](char ch) {
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
            cout << "Enter rows, columns, height, seed, and algorithm with a single space between: ";
            int rows, columns, height, seed;
            string algorithm;
            // Get user input
            cin >> rows >> columns >> height >> seed >> algorithm;
            mazes::algo mt = mazes::to_algo_from_string(algorithm);

            if (mt == mazes::algo::TOTAL) {
                cerr << "Unknown algorithm: " << algorithm << endl;
                break;
            }

            // Create the maze
            auto next_maze_ptr = mazes::factory::create(mazes::configurator().rows(rows).columns(columns).levels(height).seed(seed)._algo(mt));

            if (!next_maze_ptr.value()) {
                cerr << "Error creating maze: " << endl;
                break;
            }

            auto next_maze_ptr_s = mazes::stringz::stringify(cref(next_maze_ptr.value()));

            unordered_map<string, string> my_json_map;
            my_json_map["rows"] = to_string(rows);
            my_json_map["columns"] = to_string(columns);
            my_json_map["levels"] = to_string(height);
            my_json_map["seed"] = to_string(seed);
            my_json_map["algo"] = algorithm;
            my_json_map["str"] = next_maze_ptr_s;

            mazes::json_helper jh{};
            auto json_s = jh.from(cref(my_json_map));

            static constexpr auto content_len = 2;

            sf::Http::Request sf_post_request {"api/mazes", sf::Http::Request::Method::Post};
            sf_post_request.setHttpVersion(1, 1);
            sf_post_request.setBody(json_s);
            sf_post_request.setField("Content-Type", "application/json");
            sf_post_request.setField("Content-Length", to_string(content_len));

            response = http.sendRequest(sf_post_request);
            cout << "Response status: " << static_cast<int>(response.getStatus()) << " \nbody: " << response.getBody() << endl;
            break;
            } // case '1'
            case '2': {
            cout << "Getting all mazes from the DB...\n";
            sf::Http::Request sf_get_request {"api/mazes", sf::Http::Request::Method::Get};
            sf_get_request.setHttpVersion(1, 1);
            response = http.sendRequest(sf_get_request);
            if (response.getStatus() != sf::Http::Response::Status::Ok) {
                cerr << "Error: " << static_cast<int>(response.getStatus()) << endl;
                break;
            }
            cout << "Response from server: " << response.getBody() << endl;
            break;
            } // case '2'
            case '3': {
            cout << "Enter id for maze to delete...\n";
            int id = 0;
            cin >> id;
            sf::Http::Request sf_del_request {"api/mazes/" + to_string(id), sf::Http::Request::Method::Delete};
            sf_del_request.setHttpVersion(1, 1);
            response = http.sendRequest(sf_del_request);
            if (response.getStatus() != sf::Http::Response::Status::Ok) {
                cerr << "Error: " << static_cast<int>(response.getStatus()) << endl;
                break;
            }
            cout << "Response from server: " << response.getBody() << endl;
            break;
            }
            case '4':
            print_menu();
            break;
            case '5': {
            // Show example
            cout << "Showing example maze builder...\n\n";
            ostringstream option_1;
            option_1 << "-- Enter rows, columns, height, seed, and algorithm with a single space between. --\n\n";
            ostringstream example_user_input;
            example_user_input << "Example (ignore [] brackets):\t[1 2 3 1 dfs]\n";
            
            cout << option_1.str() << example_user_input.str();
            
            break;
            }
            case '6':
            cout << "Exiting Maze Builder...\n";
            is_running = false;
            break;
            default:
            cout << "Invalid option. Please try again.\n";
            break;
        } // switch
        print_menu();
    }; // lambda

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
        process_commands(commands, ref(is_running));
    }

    return 0;
} // main



