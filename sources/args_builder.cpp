#include "args_builder.h"

#include <regex>
#include <stdexcept>
#include <functional>
#include <cstdlib>

using namespace mazes;

args_builder::args_builder(const std::string& v, const std::string& h, int argc, char* argv[]) {
    args_map.emplace("version", v);
    args_map.emplace("help", h);
    args_map.emplace("algo", "binary_tree");
    args_map.emplace("seed", "0");
    args_map.emplace("output", "stdout");
    args_map.emplace("interactive", "0");

    this->state = gather_args(argc, argv);
}

args_builder::args_builder(const std::unordered_map<std::string, std::string>& args) 
: args_map{args} {

}

unsigned int args_builder::get_seed() const {
    return atoi(args_map.at("seed").c_str());
}

bool args_builder::is_interactive() const {
    return static_cast<bool>(atoi(args_map.at("interactive").c_str()));
}

std::string args_builder::get_version() const {
    auto itr = args_map.find("version");
    bool check = itr != args_map.end();
    if (check) {
        return itr->second;
    }
    throw std::runtime_error("Version info not provided.");
}

std::string args_builder::get_help() const {
    auto itr = args_map.find("help");
    bool check = itr != args_map.end();
    if (check) {
        return itr->second;
    }
    throw std::runtime_error("Help info not provided.");
}

std::string args_builder::get_algo() const {
    auto itr = args_map.find("algo");
    bool check = itr != args_map.end();
    if (check) {
        return itr->second;
    }
    throw std::runtime_error("Algorithm info not provided.");
}

std::string args_builder::get_output() const {
    auto itr = args_map.find("output");
    bool check = itr != args_map.end();
    if (check) {
        return itr->second;
    }
    throw std::runtime_error("Output info not provided.");
}

args_state args_builder::get_state() const noexcept {
    return this->state;
}

std::unordered_map<std::string, std::string> args_builder::get_args_map() const noexcept {
    return this->args_map;
}

// Populate the vector of strings with char-strings
args_state args_builder::gather_args(int argc, char* argv[]) {
    using namespace std;

    args_state state {args_state::READY_TO_ROCK};

    regex interactive_regex ("(^--interactive$|^\\-i$)", regex_constants::ECMAScript);
    regex seed_regex ("(^--seed=[\\d]+$|^\\-s\\s+\\d+$)", regex_constants::ECMAScript);
    regex help_regex ("(^--help$|^\\-h$)", regex_constants::ECMAScript);
    regex version_regex ("(^--version$|^\\-v$)", regex_constants::ECMAScript);
    regex algo_regex ("(^--algo=[\\w]+|^\\-a\\s+\\w+$)", regex_constants::ECMAScript);
    regex output_regex ("(^--output=[\\w\\\\.]+|^\\-o\\s+[\\w|\\\\.]+$)", regex_constants::ECMAScript);

    // skip program name
    for (unsigned int i = 1; i < argc; i++) {
        string current (argv[i]);
        if (regex_match(current, interactive_regex)) {
            args_map["interactive"] = "1";
        } else if (regex_match(current, seed_regex)) {
            // seed follows an '=' or some spaces (or just one space...)
            std::string seed_val = "";
            if (current.compare("-s") == 0) { 
                if (i + 1 < argc) {
                    seed_val = argv[i + 1];
                }
                args_map["seed"] = seed_val;
            } else {
                // start at -1 because increment in while loop
                int counter = -1;
                std::string temp = "";
                while (++counter < current.size()) {
                    static bool start_counting_digits = false;
                    if (current[counter] == '=') {
                        start_counting_digits = true;
                        continue;
                    }
                    if (start_counting_digits) {
                        temp += current[counter];
                    }
                }
                args_map["seed"] = temp;
            }
        } else if (regex_match(current, help_regex)) {
            state = args_state::JUST_NEEDS_HELP;
        } else if (regex_match(current, version_regex)) {
            state = args_state::JUST_NEEDS_VERSION;
        } else if (regex_match(current, algo_regex)) {
            args_map.emplace("algo", "binary_tree");
        } else if (regex_match(current, output_regex)) {
            args_map.emplace("output", "stdout");
        } else {
            throw runtime_error("Could not handle arguments: " + current);
        }
    }

    function<bool(const std::string&)> check_args = [this](const std::string& s) {
        auto itr = this->args_map.find(s);
        return itr == this->args_map.end();
    };

    // check if interactive was set, else set it to '0'
    bool has_arg = check_args("interactive");
    if (!has_arg) {
        args_map.emplace("interactive", "0");
    }

    // Do the same check for seed, algo, output and set defaults
    has_arg = check_args("seed");
    if (!has_arg) {
        args_map.emplace("seed", "0");
    }

    return state;
} // gather_args