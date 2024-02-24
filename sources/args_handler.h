#ifndef ARGS_HANDLER_H
#define ARGS_HANDLER_H

#include <vector>
#include <string>

class args_handler {
public:
    args_handler(int argc, char* argv[]);

    unsigned int seed;
    bool interactive;

private:
    void gather_args(int argc, char* argv[]);
    std::vector<std::string> args_in_vec;
};

#endif // ARGS_HANDLER_H