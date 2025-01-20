#ifndef BUILDER_H
#define BUILDER_H

#include <memory>

#include <MazeBuilder/enums.h>

namespace mazes {

class maze;
using maze_ptr = std::unique_ptr<maze>;

class builder {
public:
private:
    maze_ptr my_maze;
public:
    explicit builder();

    builder& rows(int rows);

    builder& columns(int columns);

    builder& height(int height);

    builder& seed(int seed);

    builder& block_type(int block_type);

    builder& maze_type(mazes::algos maze_type);

    builder& show_distances(bool show_distances);

    builder& offset_x(int s_x);

    builder& offset_z(int s_z);

    maze_ptr build() noexcept;

}; // class

} // namespace

#endif // BUILDER_H
