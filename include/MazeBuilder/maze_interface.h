#ifndef MAZE_INTERFACE_H
#define MAZE_INTERFACE_H

#include <string>

namespace mazes {

/// @file maze_interface.h
/// @class maze_interface
/// @brief Interface for the mazes
class maze_interface {

public:
    virtual ~maze_interface() = default;

    virtual std::string maze() const noexcept = 0;
}; // maze_interface

} // namespace mazes

#endif // MAZE_INTERFACE_H
