/// @file Args handles what the program can accept as input

#include <MazeBuilder/args.h>
#include <MazeBuilder/json_helper.h>

#include <string>
#include <regex>
#include <functional>
#include <algorithm>
#include <numeric>

using namespace mazes;

bool args::parse(const std::vector<std::string>& arguments) noexcept {
    using namespace std;

    if (arguments.empty()) {
        return true;
    }

    // Convert the arguments to a single string
    auto str = std::accumulate(arguments.cbegin(), arguments.cend(), std::string{},
        [](const std::string& a, const std::string& b) {
            return a.empty() ? b : a + " " + b;
        });

    return this->parse(cref(str));
}

bool args::parse(const std::string& arguments) noexcept {
    using namespace std;

    if (arguments.empty()) {
        return true;
    }

    // Find the app name
    auto pos = arguments.find(' ');
    // Return early if it's just the app name
    if (pos == std::string::npos) {
        args_map.insert_or_assign("app", arguments);
        return true;
    }

    // Start iterating after the app name
    auto it = arguments.cbegin() + pos + 1;

    // Check if the arguments without the app name are valid
    const regex pattern{ ArgsPattern };
    if (!regex_match(string(it, arguments.cend()), pattern)) {
        return false;
    }

    args_map.insert_or_assign("app", arguments.substr(0, pos));

    auto skip_spaces = [&it, &arguments]() {
        while (it != arguments.cend() && *it == ' ') {
            ++it;
        }
        };

    while (it != arguments.cend()) {
        skip_spaces();

        if (it == arguments.cend()) {
            break;
        }

        if (*it == '-' && (it + 1) != arguments.cend() && *(it + 1) == '-') {
            // Long option
            auto key_start = it + 2;
            it = std::find(key_start, arguments.cend(), '=');
            if (it != arguments.cend()) {
                std::string key(key_start, it);
                advance(it, 1);
                auto value_start = it;
                skip_spaces();
                args_map.insert_or_assign(key, std::string(value_start, it));
            } else {
                it = std::find(key_start, arguments.cend(), ' ');
                args_map.insert_or_assign(std::string(key_start, it), "");
            }
        } else if (*it == '-' && (it + 1) != arguments.cend() && *(it + 1) != '-') {
            // Short option
            std::string key(1, *(it + 1));
            advance(it, 2);
            skip_spaces();
            if (it != arguments.cend() && *it != '-') {
                auto value_start = it;
                skip_spaces();
                args_map.insert_or_assign(key, std::string(value_start, it));
            } else {
                args_map.insert_or_assign(key, "");
            }
        } else {
            advance(it, 1);
        }
    }

    return true;
}

std::string args::get(const std::string& key) const noexcept {
    auto it = args_map.find(key);
    if (it != args_map.end()) {
        return it->second;
    }
    return "";
}

const std::unordered_map<std::string, std::string>& args::get() const noexcept {
    return args_map;
}

/// @brief Dump the hash table to a string output
/// @return 
std::ostream& mazes::operator<<(std::ostream& os, const args& a) noexcept {
    if (a.args_map.empty()) {
        return os;
    }

    json_helper jh{};
    os << jh.from(a.args_map);
    return os;
}
