/*
Args builds and parses program arguments
The args have default values set in the constructor
If no map is provided in the constructor the args are "gathered" or "parsed"
The getters throw exceptions if the arg is not found, generally if a map is provided w/o default values
It is preferred to parse program arguments but the map is useful for testing
*/

#include <MazeBuilder/args_builder.h>

#include <iostream>
#include <string>
#include <regex>
#include <functional>

using namespace mazes;

/**
 * @brief Construct args from program args
 */
args_builder::args_builder(const std::vector<std::string>& vv)
: my_args{} {
    this->parse(cref(vv));
    // Apply defaults
    if (this->my_args.algorithm.empty()) {
        this->my_args.algorithm = "binary_tree";
    }
    if (this->my_args.output.empty()) {
        this->my_args.output = "stdout";
    }
	if (this->my_args.rows == 0) {
		this->my_args.rows = 100;
	}
	if (this->my_args.columns == 0) {
		this->my_args.columns = 100;
	}
	if (this->my_args.height == 0) {
		this->my_args.height = 10;
	}
}

args_builder& args_builder::seed(int seed) noexcept {
    this->my_args.seed = seed;
	return *this;
}

args_builder& args_builder::interactive(bool interactive) noexcept {
    this->my_args.interactive = interactive;
    return *this;
}

args_builder& args_builder::version(const std::string& version) noexcept {
    this->my_args.version = version;
	return *this;
}

args_builder& args_builder::help(const std::string& help) noexcept {
    this->my_args.help = help;
    return *this;
}

args_builder& args_builder::algorithm(const std::string& algorithm) noexcept {
	this->my_args.algorithm = algorithm;
	return *this;
}

args_builder& args_builder::output(const std::string& output) noexcept {
    this->my_args.output = output;
	return *this;
}

args_builder& args_builder::rows(int rows) noexcept {
    this->my_args.rows = rows;
    return *this;
}

args_builder& args_builder::columns(int columns) noexcept {
    this->my_args.columns = columns;
    return *this;
}

args_builder& args_builder::height(int height) noexcept {
	this->my_args.height = height;
	return *this;
}

args_builder& args_builder::distances(bool distances) noexcept {
	this->my_args.distances = distances;
    return *this;
}

void args_builder::clear() noexcept {
    this->my_args = args{};
}

args args_builder::build() const noexcept {
    return this->my_args;
}

/**
 * @brief Parse program arguments, store data in a stack, and then build the args
 */
void args_builder::parse(const std::vector<std::string>& vv) noexcept {
    using namespace std;

    // This function parses "long" options like "--seed=12345" or "--interactive"
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

    regex interactive_regex ("^--interactive$|^-i$", regex_constants::ECMAScript);
    regex seed_regex ("--seed=[\\d+]+|^-s$", regex_constants::ECMAScript);
    regex rows_regex ("--rows=[\\d+]+|^-r$", regex_constants::ECMAScript);
    regex columns_regex("--columns=[\\d]+|^-c$", regex_constants::ECMAScript);
    regex height_regex ("--height=[\\d]+|^-y$", regex_constants::ECMAScript);
    regex help_regex ("--help|^-h$", regex_constants::ECMAScript);
    regex version_regex ("--version|^-v$", regex_constants::ECMAScript);
    regex algo_regex ("--algorithm=[\\w]+|^-a$", regex_constants::ECMAScript);
    regex output_regex ("(^--output=[\\w\\\\.]+|^-o$)", regex_constants::ECMAScript);
    regex distances_regex("(^--distances$|^-d$)", regex_constants::ECMAScript);

    string short_val_option {""};
    // skip program name with +1
    auto&& itr = vv.cbegin() + 1;
    for (; itr != vv.cend(); itr++) {
        string current {*itr};
        if (regex_match(current, help_regex)) {
            this->my_args.help = "REPLACE_WITH_HELP_MESSAGE";
            break;
        } else if (regex_match(current, version_regex)) {
            this->my_args.version = "REPLACE_WITH_VERSION_MESSAGE";
            break;
        } else if (regex_match(current, interactive_regex)) {
            this->my_args.interactive = true;
        } else if (regex_match(current, seed_regex)) {
            // seed follows an '=' for long option, -s for short option
            if (current.compare("-s") == 0) { 
                if (itr + 1 != vv.cend()) {
                    this->my_args.seed = atoi((*(itr + 1)).c_str());
                    itr++;
                } else {
                    break;
                }    
            } else {
                this->my_args.seed = atoi(get_val_from_long_option(current).c_str());
            }
		} else if (regex_match(current, distances_regex)) {
			// --distances, -d
            this->my_args.distances = true;
        } else if (regex_match(current, rows_regex)) {
            // --rows=?, -r ?
            if (current.compare("-r") == 0) {
                if (itr + 1 != vv.cend()) {
                    this->my_args.rows = atoi((*(itr + 1)).c_str());
                    itr++;
                } else {
                    break;
                }
            } else {
                this->my_args.rows = atoi(get_val_from_long_option(current).c_str());
            }
        } else if (regex_match(current, columns_regex)) {
            // --columns=?, -c ?
            if (current.compare("-c") == 0) { 
                if (itr + 1 != vv.cend()) {
                    this->my_args.columns = atoi((*(itr + 1)).c_str());
                    itr++;
                } else {
                    break;
                }
            } else {
                this->my_args.columns = atoi(get_val_from_long_option(current).c_str());
            }
        } else if (regex_match(current, height_regex)) {
            // --height=?, -y ?
            if (current.compare("-y") == 0) { 
                if (itr + 1 != vv.cend()) {
                    this->my_args.height = atoi((*(itr + 1)).c_str());
                    itr++;
                } else {
                    break;
                }
            } else {
                this->my_args.height = atoi(get_val_from_long_option(current).c_str());
            }
        } else if (regex_match(current, algo_regex)) {
            // --algorithm=?, -a ?
            if (current.compare("-a") == 0) { 
                if (itr + 1 != vv.cend()) {
                    this->my_args.algorithm = (*(itr + 1));
                    itr++;
                } else {
                    break;
                }
            } else {
                this->my_args.algorithm = get_val_from_long_option(current);
            }
        } else if (regex_match(current, output_regex)) {
            // --output=?, -o ?
            if (current.compare("-o") == 0) { 
                if (itr + 1 != vv.cend()) {
                    this->my_args.output = (*(itr + 1));
                    itr++;
                } else {
					break;
				}
            } else {
                this->my_args.output = get_val_from_long_option(current);
            }
        } else {
#if defined(MAZE_DEBUG)
            cerr << "ERROR: Could not handle arguments: " << current << endl;
#endif
        }
    } // loop
} // parse

