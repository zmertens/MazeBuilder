#include <MazeBuilder/builder.h>

#include <MazeBuilder/maze.h>

using namespace mazes;

builder::builder() 
: my_maze(std::make_unique<maze>()) {

}

builder& builder::rows(int rows) {
    my_maze->rows = rows;
    return *this;
}

builder& builder::columns(int columns) {
    my_maze->columns = columns;
    return *this;
}

builder& builder::height(int height) {
    my_maze->height = height;
    return *this;
}

builder& builder::seed(int seed) {
    my_maze->seed = seed;
    return *this;
}

builder& builder::block_type(int block_type) {
    my_maze->block_type = block_type;
    return *this;
}

builder& builder::maze_type(mazes::algos maze_type) {
    my_maze->maze_type = maze_type;
    return *this;
}

builder& builder::show_distances(bool show_distances) {
    my_maze->distances = show_distances;
    return *this;
}

builder& builder::offset_x(int s_x) {
    my_maze->offset_x = s_x;
    return *this;
}

builder& builder::offset_z(int s_z) {
    my_maze->offset_z = s_z;
    return *this;
}

maze_ptr builder::build() noexcept {
    return std::move(my_maze);
}