#ifndef CLI_H
#define CLI_H

#include <string>
#include <memory>

#include <MazeBuilder/singleton_base.h>
#include <MazeBuilder/args.h>

class cli : public mazes::singleton_base<cli> {
    friend class mazes::singleton_base<cli>;
public:
    std::string stringify_from_args(const std::string& args) const noexcept;
    std::string stringify_from_dimens(unsigned int rows, unsigned int cols) const noexcept;
    
    // New method to handle all CLI arguments
    std::string process_command_line(const mazes::args& args) const noexcept;

private:
};


#endif // CLI_H