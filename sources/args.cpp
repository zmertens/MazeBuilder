/*
Args builds and parses program arguments
The args have default values set in the constructor
If no map is provided in the constructor the args are "gathered" or "parsed"
The getters throw exceptions if the arg is not found, generally if a map is provided w/o default values
It is preferred to parse program arguments but the map is useful for testing
*/

#include <MazeBuilder/args.h>

#include <string>
#include <regex>
#include <functional>

using namespace mazes;

/// @brief Default constructor
args::args()
: algo{"binary_tree"}
, distances{false}
, output{"stdout"}
, seed{0}
, columns{10}
, rows{10}
, height{1}
, version{"0.0.1"}
, help{"REPLACE_WITH_HELP_MESSAGE"} {

}

/**
 * @brief Parse program arguments, store data in a stack, and then build the args
 */
bool args::parse(const std::vector<std::string>& arguments) noexcept {
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
    auto&& itr = arguments.cbegin() + 1;
    for (; itr != arguments.cend(); itr++) {
        string current {*itr};
        if (regex_match(current, help_regex)) {
            break;
        } else if (regex_match(current, version_regex)) {
            break;
        } else if (regex_match(current, seed_regex)) {
            // seed follows an '=' for long option, -s for short option
            if (current.compare("-s") == 0) { 
                if (itr + 1 != arguments.cend()) {
                    this->seed = atoi((*(itr + 1)).c_str());
                    itr++;
                } else {
                    break;
                }    
            } else {
                this->seed = atoi(get_val_from_long_option(current).c_str());
            }
		} else if (regex_match(current, distances_regex)) {
			// --distances, -d
            this->distances = true;
        } else if (regex_match(current, rows_regex)) {
            // --rows=?, -r ?
            if (current.compare("-r") == 0) {
                if (itr + 1 != arguments.cend()) {
                    this->rows = atoi((*(itr + 1)).c_str());
                    itr++;
                } else {
                    break;
                }
            } else {
                this->rows = atoi(get_val_from_long_option(current).c_str());
            }
        } else if (regex_match(current, columns_regex)) {
            // --columns=?, -c ?
            if (current.compare("-c") == 0) { 
                if (itr + 1 != arguments.cend()) {
                    this->columns = atoi((*(itr + 1)).c_str());
                    itr++;
                } else {
                    break;
                }
            } else {
                this->columns = atoi(get_val_from_long_option(current).c_str());
            }
        } else if (regex_match(current, height_regex)) {
            // --height=?, -y ?
            if (current.compare("-y") == 0) { 
                if (itr + 1 != arguments.cend()) {
                    this->height = atoi((*(itr + 1)).c_str());
                    itr++;
                } else {
                    break;
                }
            } else {
                this->height = atoi(get_val_from_long_option(current).c_str());
            }
        } else if (regex_match(current, algo_regex)) {
            // --algorithm=?, -a ?
            if (current.compare("-a") == 0) { 
                if (itr + 1 != arguments.cend()) {
                    this->algo = (*(itr + 1));
                    itr++;
                } else {
                    break;
                }
            } else {
                this->algo = get_val_from_long_option(current);
            }
        } else if (regex_match(current, output_regex)) {
            // --output=?, -o ?
            if (current.compare("-o") == 0) { 
                if (itr + 1 != arguments.cend()) {
                    this->output = (*(itr + 1));
                    itr++;
                } else {
					break;
				}
            } else {
                this->output = get_val_from_long_option(current);
            }
        } else {
            return false;
        }
    } // loop

    return true;
} // parse

