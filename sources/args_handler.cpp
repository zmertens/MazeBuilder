#include "args_handler.h"

#include <regex>

args_handler::args_handler(int argc, char* argv[])
: seed(0)
, interactive(false) {
    gather_args(argc, argv);
}

// Populate the vector of strings with char-strings
void args_handler::gather_args(int argc, char* argv[]) {
    using namespace std;

    regex interactive_regex ("[--interactive|-i]{1}", regex_constants::ECMAScript);
    regex seed_regex ("[--seed=[\\d]+|-s\\w+[\\d]+]{1}", regex_constants::ECMAScript);

    // skip program name
    for (unsigned int i = 1; i < argc; i++) {
        this->args_in_vec.emplace_back(argv[i]);
        string last = args_in_vec.back();
        if (regex_search(last, interactive_regex)) {
            this->interactive = true;
        }
    }
}