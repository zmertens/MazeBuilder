#ifndef ARGS_HANDLER_H
#define ARGS_HANDLER_H

#include <string>
#include <sstream>
#include <unordered_map>
#include <ostream>
#include <vector>
#include <functional>

#include "args_builder_interface.h"

namespace mazes {

enum class args_state {
    JUST_NEEDS_HELP, JUST_NEEDS_VERSION, READY_TO_ROCK
};

class args_builder : public args_builder_interface {
public:
    args_builder(const std::string& v, const std::string& h, const std::vector<std::string>& args_vec);
    args_builder(const std::unordered_map<std::string, std::string>& args);

    unsigned int get_seed() const noexcept;
    bool is_interactive() const noexcept;
    std::string get_version() const noexcept;
    std::string get_help() const noexcept;
    std::string get_algorithm() const noexcept;
    std::string get_output() const noexcept;
    unsigned int get_width() const noexcept;
    unsigned int get_length() const noexcept;
    unsigned int get_height() const noexcept;
    
    args_state get_state() const noexcept;

    const std::unordered_map<std::string, std::string>& build() override;

    friend std::ostream& operator<<(std::ostream& os, args_builder& args) {
        std::stringstream ss;
        for (auto&& [k, v] : args.build()) {
            ss << "INFO: " << k << ", " << v << "\n";
        }

        return os << ss.str() << "\n";
    }

private:
    void gather_args();
    std::unordered_map<std::string, std::string> args_map;
    std::vector<std::string> args_vec;
};

}
#endif // ARGS_HANDLER_H
