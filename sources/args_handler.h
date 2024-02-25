#ifndef ARGS_HANDLER_H
#define ARGS_HANDLER_H

#include <string>
#include <unordered_map>

class args_handler {
public:
    args_handler(const std::string& v, const std::string& h, int argc, char* argv[]);

    unsigned int get_seed() const;
    bool is_interactive() const;
    std::string get_version() const;
    std::string get_help() const;
    std::string get_algo() const;
    std::string get_output() const;

private:
    void gather_args(int argc, char* argv[]);
    std::unordered_map<std::string, std::string> args_map;
};

#endif // ARGS_HANDLER_H