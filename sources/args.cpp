#include <MazeBuilder/args.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/writer.h>

#include <string>
#include <regex>
#include <functional>
#include <sstream>
#include <algorithm>

using namespace mazes;

bool args::parse(const std::vector<std::string>& arguments) noexcept {
    using namespace std;

    // Short argument regex for -x or -y or -z
    const regex short_arg_regex{ R"(-[a-zA-Z]\s*\d*)" };
    const regex long_arg_regex{ R"(--[a-zA-Z]+(\s*=\s*\S+)?)" };
    // Combined short arguments can represent -x1 or -y2 or -z3 
    const regex combined_short_arg_regex{ R"(-[a-zA-Z]{2,})" };
    const regex json_arg_regex{ R"(--json=\S+|-j|-j\s*)" };

    for (const auto& arg : arguments) {
        if (arg.empty()) {
            continue;
        }

        auto arg_t = trim(arg);

        smatch match;

        // Check for JSON input first
        if (regex_match(arg_t, match, json_arg_regex)) {
            string key = match.str(0);
            string value = "";
            auto pos = key.find('=');
            if (pos != string::npos) {
                value = key.substr(pos + 1);
                key = key.substr(0, pos);
            } else if (key.find('j') != string::npos) {
                value = trim(arguments.cend()[-1]);
                key = "j";
            }
            args_map[key] = trim(value);
            json_helper jh{};
            return jh.load(value, args_map);
        } else if (regex_match(arg_t, match, long_arg_regex)) {
            string key = match.str(0);
            string value = "";
            auto pos = key.find('=');
            if (pos != string::npos) {
                value = key.substr(pos + 1);
                key = key.substr(0, pos);
            }
            args_map[key] = trim(value);
        } else if (regex_match(arg_t, match, short_arg_regex)) {
            string key = match.str(0);
            string value = "";
            if (key.length() > 2) {
                value = key.substr(2);
                key = key.substr(0, 2);
            }
            args_map[key] = trim(value);
        } else if (regex_match(arg_t, match, combined_short_arg_regex)) {
            for (size_t i = 1; i < match.str(0).length(); ++i) {
                args_map[string(1, match.str(0)[i])] = "";
            }
        } else {
            return false;
        }
    }

    return true;
}

bool args::parse(const std::string& arguments) noexcept {
    std::istringstream iss(arguments);
    std::vector<std::string> args_vec((std::istream_iterator<std::string>(iss)),
        std::istream_iterator<std::string>());
    return parse(args_vec);
}

void args::clear() noexcept {
    args_map.erase(args_map.begin(), args_map.end());
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

std::string args::trim(const std::string& str) const noexcept {
    if (str.empty()) {
        return str;
    }
    auto start = str.find_first_not_of(" \t");
    auto end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
}

/// @brief Dump the hash table to a string output
/// @return 
std::ostream& mazes::operator<<(std::ostream& os, const args& a) noexcept {
    if (a.args_map.empty() || !os.good()) {
        return os;
    }

    json_helper jh{};
    os << jh.from(a.args_map);
    return os;
}
