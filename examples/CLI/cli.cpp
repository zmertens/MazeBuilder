#include "cli.h"

#include <MazeBuilder/maze_builder.h>

std::string cli::stringify_from_args(const std::string& args) const noexcept {
    using namespace std;
    using namespace mazes;

    
}

std::string cli::stringify_from_dimens(unsigned int rows, unsigned int cols) const noexcept {
    using namespace std;
    using namespace mazes;

    factory my_factory{};
    auto maze_ptr = my_factory.create(configurator().rows(rows).columns(cols));
    return stringz::stringify(cref(maze_ptr.value()));
}
