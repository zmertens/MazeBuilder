#ifndef CLI_H
#define CLI_H

#include <string>
#include <memory>

#include <MazeBuilder/singleton_base.h>

class cli : public mazes::singleton_base<cli> {
    friend class mazes::singleton_base<cli>;
public:
    std::string stringify_from_args(const std::string& args) const noexcept;
    std::string stringify_from_dimens(unsigned int rows, unsigned int cols) const noexcept;


private:
};


#endif // CLI_H