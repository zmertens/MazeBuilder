#ifndef ARGS_HANDLER_H
#define ARGS_HANDLER_H

#include <string>
#include <sstream>
#include <unordered_map>
#include <ostream>
#include <functional>

#include "args_builder_interface.h"

namespace mazes {

enum class args_state {
    JUST_NEEDS_HELP, JUST_NEEDS_VERSION, READY_TO_ROCK
};

class args_builder : public args_builder_interface {
public:
    args_builder(const std::string& v, const std::string& h, int argc, char* argv[]);
    args_builder(const std::unordered_map<std::string, std::string>& args);

    unsigned int get_seed() const;
    bool is_interactive() const;
    std::string get_version() const;
    std::string get_help() const;
    std::string get_algo() const;
    std::string get_output() const;
    unsigned int get_width() const;
    unsigned int get_length() const;
    unsigned int get_height() const;
    
    args_state get_state() const noexcept;

    const std::unordered_map<std::string, std::string>& build() const noexcept override;

    friend std::ostream& operator<<(std::ostream& os, args_builder& args) {
        std::stringstream ss;
        for (auto&& k : args.build()) {
            ss << "INFO: " << k.first << ", " << k.second << "\n";
        }

        return os << ss.str() << "\n";
    }

private:
    void gather_args(int argc, char* argv[]);
    std::unordered_map<std::string, std::string> args_map;
    args_state state;

    std::function<bool(const std::string&)> has_args;
};

}
#endif // ARGS_HANDLER_H
