#include "args_handler.h"

#include <regex>
#include <exception>

#include <SDL3/SDL.h>

args_handler::args_handler(const std::string& v, const std::string& h, int argc, char* argv[]) {
    args_map.emplace("version", v);
    args_map.emplace("help", h);

    gather_args(argc, argv);
}

unsigned int args_handler::get_seed() const {
    return SDL_atoi(args_map.at("seed").c_str());
}

bool args_handler::is_interactive() const {
    return static_cast<bool>(SDL_atoi(args_map.at("interactive").c_str()));
}

std::string args_handler::get_version() const {
    auto itr = args_map.find("version");
    bool check = itr != args_map.end();
    if (check) {
        return itr->second;
    }
}

std::string args_handler::get_help() const {
    auto itr = args_map.find("help");
    bool check = itr != args_map.end();
    if (check) {
        return itr->second;
    }
}

std::string args_handler::get_algo() const {
    auto itr = args_map.find("algo");
    bool check = itr != args_map.end();
    if (check) {
        return itr->second;
    }
}

std::string args_handler::get_output() const {
    auto itr = args_map.find("output");
    bool check = itr != args_map.end();
    if (check) {
        return itr->second;
    }
}

// Populate the vector of strings with char-strings
void args_handler::gather_args(int argc, char* argv[]) {
    using namespace std;

    regex interactive_regex ("(^--interactive$|^\\-i$)", regex_constants::ECMAScript);
    regex seed_regex ("(^--seed=[\\d]+$|^\\-s\\s+\\d+$)", regex_constants::ECMAScript);
    regex help_regex ("(^--help$|^\\-h$)", regex_constants::ECMAScript);
    regex version_regex ("(^--version$|^\\-v$)", regex_constants::ECMAScript);
    regex algo_regex ("(^--algo=[\\w]+|^\\-a\\s+\\w+$)", regex_constants::ECMAScript);
    regex output_regex ("(^--output=[\\w\\\\.]+|^\\-o\\s+[\\w|\\\\.]+$)", regex_constants::ECMAScript);

    // skip program name
    for (unsigned int i = 1; i < argc; i++) {
#if defined(DEBUGGING)
        SDL_Log("parsing args: %s\n", argv[i]);
#endif
        string current (argv[i]);
        if (regex_match(current, interactive_regex)) {
            args_map.emplace("interactive", "1");
        } else if (regex_match(current, seed_regex)) {
            
        } else if (regex_match(current, help_regex)) {
            throw runtime_error(this->get_help());
            break;
        } else if (regex_match(current, version_regex)) {
            throw runtime_error(this->get_version());
            break;
        } else if (regex_match(current, algo_regex)) {
            
        } else if (regex_match(current, output_regex)) {
            
        } else {
            throw runtime_error("Could not handle arguments: " + current);
        }
    }

    // check if interactive was set, else set it to '0'
    auto itr = args_map.find("interactive");
    bool not_interactive = itr == args_map.end();
    if (not_interactive) {
        args_map.emplace("interactive", "0");
    }

    // Do the same check for seed, algo, output and set defaults
    itr = args_map.find("seed");
    bool no_seed = itr == args_map.end();
    if (no_seed) {
        args_map.emplace("seed", "0");
    }

    itr = args_map.find("algo");
    bool no_algo = itr == args_map.end();
    if (no_algo) {
        args_map.emplace("algo", "bst");
    }

    itr = args_map.find("output");
    bool no_output = itr == args_map.end();
    if (no_output) {
        args_map.emplace("output", "stdout");
    }
} // gather_args