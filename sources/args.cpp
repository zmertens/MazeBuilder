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
    if (pos == string::npos) {
        args_map.insert_or_assign("app", arguments);
        return true;
    }

    args_map.insert_or_assign("app", arguments.substr(0, pos));

    // Check that the argument options are a valid pattern
    const regex args_pattern{ args::ArgsPattern };
    auto t = arguments.substr(pos + 1);
    if (!regex_match(t, args_pattern)) {
        return false;
    }

    // Transform the argument string into map entries
    auto it = arguments.cbegin() + pos + 1;
    while (it != arguments.cend()) {
        if (*it == ' ') {
            ++it;
            continue;
        }

        if (*it == '-' && (it + 1) != arguments.cend() && *(it + 1) != '-') {
            // Short option
            string key(1, *(it + 1));
            it += 2;
            while (it != arguments.cend() && *it == ' ') {
                ++it;
            }
            if (it != arguments.cend() && *it != '-') {
                auto value_start = it;
                while (it != arguments.cend() && *it != ' ') {
                    ++it;
                }
                args_map.insert_or_assign(key, string(value_start, it));
            } else {
                args_map.insert_or_assign(key, "");
            }
        } else if (it + 1 != arguments.cend() && *it == '-' && *(it + 1) == '-') {
            // Long option
            auto key_start = it + 2;
            it = find(it, arguments.cend(), '=');
            if (it != arguments.cend()) {
                string key(key_start, it);
                ++it;
                auto value_start = it;
                while (it != arguments.cend() && *it != ' ') {
                    ++it;
                }
                args_map.insert_or_assign(key, string(value_start, it));
            } else {
                it = find(key_start, arguments.cend(), ' ');
                args_map.insert_or_assign(string(key_start, it), "");
            }
        } else {
            ++it;
        }
    } // while

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
    json_helper jh{};
    os << jh.from(a.args_map);
    return os;
}
