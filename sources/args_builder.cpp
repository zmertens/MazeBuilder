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

args_builder::args_builder(const std::string& v, const std::string& h, const std::vector<std::string>& args_vec)
: state {args_state::READY_TO_ROCK}
, args_vec{args_vec} {
    args_map.emplace("version", v);
    args_map.emplace("help", h);
    args_map.emplace("algorithm", "binary_tree");
    args_map.emplace("seed", "0");
    args_map.emplace("output", "stdout");
    args_map.emplace("interactive", "0");
    args_map.emplace("width", "100");
    args_map.emplace("length", "100");
    args_map.emplace("height", "10");

}

args_builder::args_builder(const std::unordered_map<std::string, std::string>& args) 
: state {args_state::READY_TO_ROCK}
, args_map{args}
, args_vec{} {

}

unsigned int args_builder::get_seed() const noexcept {
    return atoi(args_map.at("seed").c_str());
}

bool args_builder::is_interactive() const noexcept {
    return static_cast<bool>(atoi(args_map.at("interactive").c_str()));
}

std::string args_builder::get_version() const noexcept {
    return args_map.at("version");
}

std::string args_builder::get_help() const noexcept {
    return args_map.at("help");
}

std::string args_builder::get_algo() const noexcept {
    return this->args_map.at("algorithm");
}

std::string args_builder::get_output() const noexcept {
    return this->args_map.at("output");
}

args_state args_builder::get_state() const noexcept {
    return this->state;
}

unsigned int args_builder::get_width() const noexcept {
    return atoi(this->args_map.at("width").c_str());
}

unsigned int args_builder::get_length() const noexcept {
    return atoi(this->args_map.at("length").c_str());
}

unsigned int args_builder::get_height() const noexcept {
    return atoi(this->args_map.at("height").c_str());
}

// parse args before returning the map if it's non-empty
const std::unordered_map<std::string, std::string>& args_builder::build() {
    if (!args_vec.empty())
        gather_args();
    return args_map;
}

// Populate the args map
void args_builder::gather_args() {
    using namespace std;

    // This function parse "long" options like "--seed=12345"
    // by only collecting the digits, or characters like "--algorithm=binary_tree"
    auto get_val_from_long_option = [&](const string& current) {
        // start at -1 because increment in while loop
        int counter = -1;
        string temp = "";
        while (++counter < current.size()) {
            if (current[counter] == '=' && counter + 1 < current.size()) {
                temp.append(current.substr(counter + 1, current.size()));
                break;
            }
        }
        return temp;
    };

    // For short options like "-s 42", there are two passes that need to happen
    // When "-s" is found is stored in a bool value, and when the digits are found
    // in the next pass they are passed into the short val function and state is updated
    bool needs_short_val = false;
    auto set_val_from_short_option = [this, &needs_short_val](const string& option, const string& value) {
        if (needs_short_val) {
            needs_short_val = !needs_short_val;
            this->args_map[option] = value;
        }
    };

    regex interactive_regex ("^--interactive$|^-i$", regex_constants::ECMAScript);
    regex seed_regex ("--seed=[\\d+]+|^-s$", regex_constants::ECMAScript);
    regex width_regex ("--width=[\\d+]+|^-w$", regex_constants::ECMAScript);
    regex length_regex ("--length=[\\d]+|^-l$", regex_constants::ECMAScript);
    // short option -h is changed because of the --help short option
    regex height_regex ("--height=[\\d]+|^-y$", regex_constants::ECMAScript);
    regex help_regex ("(^--help$|^\\-h$)", regex_constants::ECMAScript);
    regex version_regex ("(^--version$|^\\-v$)", regex_constants::ECMAScript);
    regex algo_regex ("--algorithm=[\\w]+|^-a$", regex_constants::ECMAScript);
    regex output_regex ("(^--output=[\\w\\\\.]+|^\\-o$)", regex_constants::ECMAScript);

    string short_val_option {""};
    // skip program name with +1
    for (auto&& itr {args_vec.cbegin() + 1}; itr != args_vec.cend(); itr++) {
        string current {*itr};
        // check if we are just getting short option value
        if (needs_short_val) {
            set_val_from_short_option(short_val_option, current);
            continue;
        }

        if (regex_match(current, help_regex)) {
            this->state = args_state::JUST_NEEDS_HELP;
            break;
        } else if (regex_match(current, version_regex)) {
            this->state = args_state::JUST_NEEDS_VERSION;
            break;
        } else if (regex_match(current, interactive_regex)) {
            args_map["interactive"] = "1";
        } else if (regex_match(current, seed_regex)) {
            // seed follows an '=' for long option, -s for short option
            if (current.compare("-s") == 0) { 
                if (itr + 1 != args_vec.cend()) {
                    short_val_option = "seed";
                    needs_short_val = true;
                } else {
                    throw runtime_error("no value provided for -s: " + current);
                }    
            } else {
                args_map["seed"] = get_val_from_long_option(current);
            }
        } else if (regex_match(current, width_regex)) {
            // width follows an '=' or some spaces (or just one space...)
            if (current.compare("-w") == 0) { 
                if (itr + 1 != args_vec.cend()) {
                    short_val_option = "width";
                    needs_short_val = true;
                } else {
                    throw runtime_error("no value provided for -w: " + current);
                }
            } else {
                args_map["width"] = get_val_from_long_option(current);
            }
        } else if (regex_match(current, length_regex)) {
            // length follows an '=' or some spaces (or just one space...)
            if (current.compare("-l") == 0) { 
                if (itr + 1 != args_vec.cend()) {
                    short_val_option = "length";
                    needs_short_val = true;
                } else {
                    throw runtime_error("ERROR: no value provided for -l: " + current);
                }
            } else {
                args_map["length"] = get_val_from_long_option(current);
            }
        } else if (regex_match(current, height_regex)) {
            // height follows an '=' or some spaces (or just one space...)
            if (current.compare("-y") == 0) { 
                if (itr + 1 != args_vec.cend()) {
                    short_val_option = "height";
                    needs_short_val = true;
                } else {
                    throw runtime_error("ERROR: no value provided for -y: " + current);
                }
            } else {
                args_map["height"] = get_val_from_long_option(current);
            }
        } else if (regex_match(current, algo_regex)) {
            if (current.compare("-a") == 0) { 
                if (itr + 1 != args_vec.cend()) {
                    short_val_option = "algorithm";
                    needs_short_val = true;
                } else {
                    throw runtime_error("ERROR: no value provided for -a: " + current);
                }
            } else {
                args_map["algorithm"] = get_val_from_long_option(current);
            }
        } else if (regex_match(current, output_regex)) {
            if (current.compare("-o") == 0) { 
                if (itr + 1 != args_vec.cend()) {
                    short_val_option = "output";
                    needs_short_val = true;
                } else {
                    throw runtime_error("ERROR: no value provided for -o: " + current);
                }
            } else {
                args_map["output"] = get_val_from_long_option(current);
            }
        } else {
            throw runtime_error("ERROR: Could not handle arguments: " + current);
        }
    } // loop

    // no need to verify args here, they should have default values set in constructor
} // gather_args
