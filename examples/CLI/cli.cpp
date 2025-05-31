#include "cli.h"

#include <string>

#include <MazeBuilder/maze_builder.h>

std::string cli::stringify_from_args(const std::string& args) const noexcept {
    using namespace std;

    mazes::args a;
    if (!a.parse(args)) {

        return "Invalid arguments";
    }

    return stringify_from_dimens(stoi(a.args_map["rows"]), stoi(a.args_map["columns"]));
}

std::string cli::stringify_from_dimens(unsigned int rows, unsigned int cols) const noexcept {
    using namespace std;
    using namespace mazes;

    factory my_factory{};
    auto maze_ptr = my_factory.create(configurator().rows(rows).columns(cols));
    return maze_ptr->str().data();
}
