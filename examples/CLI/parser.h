#ifndef PARSER_H
#define PARSER_H

#include <MazeBuilder/configurator.h>

#include <string>
#include <vector>

class parser {

public:
    bool parse(std::vector<std::string> const& args, mazes::configurator& config) const;

private:

};

#endif // PARSER_H
