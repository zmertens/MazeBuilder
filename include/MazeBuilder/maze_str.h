#ifndef MAZE_STR_H
#define MAZE_STR_H

#include <MazeBuilder/maze_interface.h>

#include <memory>
#include <string>

namespace mazes {

struct maze_str : public maze_interface {

    std::string data;

    explicit maze_str(std::string d) : data(std::move(d)) {}

    std::string maze() const noexcept override {

        return data;
    }

};

} // namespace mazes

#endif // MAZE_STR_H
