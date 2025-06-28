#include "cli.h"

#include <string>

#include <MazeBuilder/maze_builder.h>

std::string cli::stringify_from_args(const std::string& args) const noexcept {
    using namespace std;

    mazes::args a;
    if (!a.parse(args)) {
        return "Invalid arguments";
    }

    // Now use the more comprehensive processor
    return process_command_line(a);
}

std::string cli::stringify_from_dimens(unsigned int rows, unsigned int cols) const noexcept {
    using namespace std;
    using namespace mazes;

    factory my_factory{};
    auto maze_ptr = my_factory.create(configurator().rows(rows).columns(cols));
    return maze_ptr->str().data();
}

std::string cli::process_command_line(const mazes::args& a) const noexcept {
    using namespace std;
    using namespace mazes;

    // Extract parameters with proper checks for both short and long form
    int rows = 10;
    int columns = 10;
    int seed = 0;
    string algo_str = "dfs";
    bool distances = false;
    
    // Check both short and long form arguments
    if (a.get("-r").has_value()) {
        rows = stoi(a.get("-r").value());
    } else if (a.get("--rows").has_value()) {
        rows = stoi(a.get("--rows").value());
    } else if (a.get("rows").has_value()) {
        rows = stoi(a.get("rows").value());
    }
    
    if (a.get("-c").has_value()) {
        columns = stoi(a.get("-c").value());
    } else if (a.get("--columns").has_value()) {
        columns = stoi(a.get("--columns").value());
    } else if (a.get("columns").has_value()) {
        columns = stoi(a.get("columns").value());
    }
    
    if (a.get("-s").has_value()) {
        seed = stoi(a.get("-s").value());
    } else if (a.get("--seed").has_value()) {
        seed = stoi(a.get("--seed").value());
    } else if (a.get("seed").has_value()) {
        seed = stoi(a.get("seed").value());
    }
    
    if (a.get("--algo").has_value()) {
        algo_str = a.get("--algo").value();
    } else if (a.get("algo").has_value()) {
        algo_str = a.get("algo").value();
    }
    
    if (a.get("-d").has_value() || a.get("--distances").has_value() || a.get("distances").has_value()) {
        distances = true;
    }
    
    // Create maze with the extracted parameters
    factory my_factory{};
    auto maze_ptr = my_factory.create(
        configurator()
        .rows(rows)
        .columns(columns)
        .seed(seed)
        .distances(distances)
        .algo_id(to_algo_from_string(algo_str))
    );
    
    return maze_ptr->str().data();
}
