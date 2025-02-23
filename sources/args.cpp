/// @file Args handles what the program can accept as input

#include <MazeBuilder/args.h>
#include <MazeBuilder/json_helper.h>

#include <string>
#include <regex>
#include <functional>
#include <numeric>

using namespace mazes;

bool args::parse(const std::vector<std::string>& arguments) noexcept {
    using namespace std;

    if (arguments.empty()) {
        return true;
    }

    // Store the executable name
    auto it = arguments.cbegin();
    args_map.insert_or_assign("app", *it);
    it++;

    // Return early if it's just the app name
    if (it == arguments.cend()) {
        return true;
    }

    // Convert the arguments to a single string
    auto str = std::accumulate(it, arguments.cend(), std::string{},
        [](const std::string& a, const std::string& b) {
            return a.empty() ? b : a + " " + b;
        });

    regex args_pattern{ args::ArgsPattern };

    // Check the argument string is valid
    if (!regex_match(str, args_pattern)) {
        // Invalid argument
        return false;
    }

    // Helper regexes for parsing
    regex distances_pattern{ R"pattern(-d|--distances)pattern" };

    auto is_distances_opt = [&distances_pattern](auto s) {
        if (regex_match(s, distances_pattern)) {
            return true;
        }
        return false;
        };

    // Transform the argument string into accurate map entries
    for (it; it != arguments.cend(); ++it) {

        if (*it == " ") {
            continue;
        }

        // Is it a short option?
        if (it->at(0) == '-' && (it + 1) != arguments.cend() && it->at(1) != '-') {
            if (is_distances_opt(*it)) {
                args_map.insert_or_assign(*(it + 1), "");
                continue;
            }
            string key = it->substr(1);
            advance(it, 1);
            // Check if short option has space(s) after it
            while (*it == " ") {
                it++;
            }
            args_map.insert_or_assign(key, *it);
        } else if (it->size() > 2 && it->at(0) == '-' && it->at(1) == '-') {
            if (is_distances_opt(*it)) {
                args_map.insert_or_assign(it->substr(2), "");
                continue;
            } else {
                // Parsing long option
                string key = it->substr(2);
                // Move past '=' sign
                advance(it, 2);
                if (*it == " ") {
                    return false;
                }
                args_map.insert_or_assign(key, *it);
            }
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
    json_helper jh{};
    os << jh.from(a.args_map);
    return os;
}
