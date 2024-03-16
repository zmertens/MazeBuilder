/*
Args builds and parses program arguments
The args have default values set in the constructor
If no map is provided in the constructor the args are "gathered" or "parsed"
The getters throw exceptions if the arg is not found, generally if a map is provided w/o default values
It is preferred to parse program arguments but the map is useful for testing
*/

#include "args_builder.h"

#include <string>
#include <regex>
#include <stdexcept>
#include <functional>
#include <cstdlib>

using namespace mazes;

args_builder::args_builder(const std::string& v, const std::string& h, int argc, char* argv[])
: state {args_state::READY_TO_ROCK}
, has_args{[this](const std::string& s) {
    return this->args_map.find(s) != this->args_map.end();}} {
    args_map.emplace("version", v);
    args_map.emplace("help", h);
    args_map.emplace("algorithm", "binary_tree");
    args_map.emplace("seed", "0");
    args_map.emplace("output", "stdout");
    args_map.emplace("interactive", "0");
    args_map.emplace("width", "100");
    args_map.emplace("length", "100");
    args_map.emplace("height", "10");

    this->gather_args(argc, argv);
}

args_builder::args_builder(const std::unordered_map<std::string, std::string>& args) 
: state {args_state::READY_TO_ROCK}
, args_map{args} {

}

unsigned int args_builder::get_seed() const {
    if (this->args_map.find("seed") == args_map.end()) {
       throw std::runtime_error("Seed not provided."); 
    }

    return atoi(args_map.at("seed").c_str());
}

bool args_builder::is_interactive() const {
    if (this->args_map.find("interactive") == args_map.end()) {
       throw std::runtime_error("Interactive option not provided."); 
    }

    return static_cast<bool>(atoi(args_map.at("interactive").c_str()));
}

std::string args_builder::get_version() const {
    if (this->args_map.find("version") == args_map.end()) {
        throw std::runtime_error("Version info not provided.");
    }
    return args_map.at("version");
}

std::string args_builder::get_help() const {
    if (this->args_map.find("help") == args_map.end()) {
        throw std::runtime_error("Help info not provided.");
    }
    return args_map.at("help");
}

std::string args_builder::get_algo() const {
    if (this->args_map.find("algorithm") == args_map.end()) {
        throw std::runtime_error("Algorithm info not provided.");
    }
    return this->args_map.at("algorithm");
}

std::string args_builder::get_output() const {
    if (this->args_map.find("output") == args_map.end()) {
        throw std::runtime_error("Output info not provided.");
    }
    return this->args_map.at("output");
}

args_state args_builder::get_state() const noexcept {
    return this->state;
}

unsigned int args_builder::get_width() const {
    if (this->args_map.find("width") == args_map.end()) {
        throw std::runtime_error("Width of maze not provided.");
    }
    return atoi(this->args_map.at("width").c_str());
}

unsigned int args_builder::get_length() const {
    if (this->args_map.find("length") == args_map.end()) {
        throw std::runtime_error("Length of maze not provided.");
    }
    return atoi(this->args_map.at("length").c_str());
}

unsigned int args_builder::get_height() const {
    if (this->args_map.find("height") == args_map.end()) {
        throw std::runtime_error("Height of maze not provided.");
    }
    return atoi(this->args_map.at("height").c_str());
}

const std::unordered_map<std::string, std::string>& args_builder::build() const noexcept {
    return this->args_map;
}

// Populate the args map
void args_builder::gather_args(int argc, char* argv[]) {
    using namespace std;

    // This function parse "long" options like "--seed=12345"
    // by only collecting the digits, or characters like "--algorithm=binary_tree"
    auto get_val_from_long_option = [&](const std::string& current) {
        // start at -1 because increment in while loop
        int counter = -1;
        std::string temp = "";
        while (++counter < current.size()) {
            static bool start_counting = false;
            if (current[counter] == '=') {
                start_counting = true;
                continue;
            }
            if (start_counting) {
                temp += current[counter];
            }
        }
        return temp;
    };

    regex interactive_regex ("(^--interactive$|^\\-i$)", regex_constants::ECMAScript);
    regex seed_regex ("^--[seed=\\d+]+|-[s\\s+\\d+]+$", regex_constants::ECMAScript);
    regex width_regex ("^--[width=\\d+]+|-[w\\s+\\d+]+$", regex_constants::ECMAScript);
    regex length_regex ("^--[length=\\d+]+|-[l\\s+\\d+]+$", regex_constants::ECMAScript);
    regex height_regex ("^--[height=\\d+]+|-[h\\s+\\d+]+$", regex_constants::ECMAScript);
    regex help_regex ("(^--help$|^\\-h$)", regex_constants::ECMAScript);
    regex version_regex ("(^--version$|^\\-v$)", regex_constants::ECMAScript);
    regex algo_regex ("(^--algorithm=[\\w]+|^\\-a\\s+\\w+$)", regex_constants::ECMAScript);
    regex output_regex ("(^--output=[\\w\\\\.]+|^\\-o\\s+[\\w|\\\\.]+$)", regex_constants::ECMAScript);

    // skip program name
    for (unsigned int i = 1; i < argc; i++) {
        string current (argv[i]);
        if (regex_match(current, help_regex)) {
            this->state = args_state::JUST_NEEDS_HELP;
            break;
        } else if (regex_match(current, version_regex)) {
            this->state = args_state::JUST_NEEDS_VERSION;
            break;
        } else if (regex_match(current, interactive_regex)) {
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
                args_map["seed"] = get_val_from_long_option(current);
            }
        } else if (regex_match(current, width_regex)) {
            // width follows an '=' or some spaces (or just one space...)
            std::string width_val = "";
            if (current.compare("-w") == 0) { 
                if (i + 1 < argc) {
                    width_val = argv[i + 1];
                }
                args_map["width"] = width_val;
            } else {
                args_map["width"] = get_val_from_long_option(current);
            }
        } else if (regex_match(current, length_regex)) {
            // length follows an '=' or some spaces (or just one space...)
            std::string length_val = "";
            if (current.compare("-l") == 0) { 
                if (i + 1 < argc) {
                    length_val = argv[i + 1];
                }
                args_map["length"] = length_val;
            } else {
                args_map["length"] = get_val_from_long_option(current);
            }
        } else if (regex_match(current, height_regex)) {
            // height follows an '=' or some spaces (or just one space...)
            std::string height_val = "";
            if (current.compare("-l") == 0) { 
                if (i + 1 < argc) {
                    height_val = argv[i + 1];
                }
                args_map["height"] = height_val;
            } else {
                args_map["height"] = get_val_from_long_option(current);
            }
        } else if (regex_match(current, algo_regex)) {
            std::string algo_val = "";
            if (current.compare("-a") == 0) { 
                if (i + 1 < argc) {
                    algo_val = argv[i + 1];
                }
                args_map["algorithm"] = algo_val;
            } else {
                args_map["algorithm"] = get_val_from_long_option(current);
            }
        } else if (regex_match(current, output_regex)) {
            std::string out_val = "";
            if (current.compare("-o") == 0) { 
                if (i + 1 < argc) {
                    out_val = argv[i + 1];
                }
                args_map["output"] = out_val;
            } else {
                args_map["output"] = get_val_from_long_option(current);
            }
        } else {
            throw runtime_error("Could not handle arguments: " + current);
        }
    }

    // no need to verify args here, they should have default values set in constructor
} // gather_args
