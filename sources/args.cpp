#include <MazeBuilder/args.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/writer.h>

#include <string>
#include <regex>
#include <functional>
#include <sstream>
#include <iterator>
#include <algorithm>

using namespace mazes;

bool args::parse(const std::vector<std::string>& arguments) noexcept {
    using namespace std;

    // Short argument regex for -x or -y or -z
    const regex short_arg_regex{ R"(-[a-zA-Z])" };
    const regex long_arg_regex{ R"(--[a-zA-Z]+\=?\S*)" };
    // Combined short arguments can represent -x1 or -y2 or -z3 
    const regex combined_short_arg_regex{ R"(-[a-zA-Z]{2,})" };
    const regex json_arg_regex{ R"(--json=\S+|-j|-j\s*)" };

    auto extract_json_str = [this, &arguments](auto is_long) -> string {
        string raw_literal = "";
        for (const auto& arg : arguments) {
            raw_literal += trim(arg);
        }

        // Remove flags
        if (is_long) {
            auto pos = raw_literal.find_first_of("=");
            raw_literal = raw_literal.substr(pos + 1);
        } else {
            auto pos = raw_literal.find_first_of("j");
            raw_literal = raw_literal.substr(pos + 1);
        }

        raw_literal.erase(std::remove(raw_literal.begin(), raw_literal.end(), '`'), raw_literal.end());
        return raw_literal;
        };

    for (size_t i = 0; i < arguments.size(); ++i) {
        const auto& arg = arguments[i];
        if (arg.empty()) {
            continue;
        }

        auto arg_t = trim(arg);

        smatch match;

        // Check for JSON input first
        if (regex_match(arg_t, match, json_arg_regex)) {
            // Counter to ensure that JSON string input is valid (enclosing backticks)
            auto backtick_counter = count_if(arguments.cbegin(), arguments.cend(), [](const auto& s) {
                return count(s.cbegin(), s.cend(), '`');
                });

            string key = match.str(0);
            string value = "";

            // Check for long-option JSON input
            json_helper jh{};
            if (key.find("--json") != string::npos) {
                auto pos = key.find('=');
                if (pos != string::npos) {
                    // Check if loading a JSON file
                    if (backtick_counter == 2 || backtick_counter == 0) {
                        key = key.substr(0, pos);
                        value = extract_json_str(true);
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            } else {
                // Check for short-option JSON input
                if (backtick_counter == 2 || backtick_counter == 0) {
                    value = extract_json_str(false);
                } else {
                    return false;
                }
            }

            args_map[key] = trim(value);
            return (backtick_counter == 2) ? jh.from(value, args_map) : jh.load(value, args_map);
        } else if (regex_match(arg_t, match, long_arg_regex)) {
            string key = match.str(0);
            string value = "";
            auto pos = key.find('=');
            if (pos != string::npos) {
                value = key.substr(pos + 1);
                key = key.substr(0, pos);
            } else if (i + 1 < arguments.size() && !regex_match(arguments[i + 1], short_arg_regex) && !regex_match(arguments[i + 1], long_arg_regex)) {
                value = arguments[++i];
            }
            args_map[key] = trim(value);
        } else if (regex_match(arg_t, match, short_arg_regex)) {
            string key = match.str(0);
            string value = "";
            if (i + 1 < arguments.size() && !regex_match(arguments[i + 1], short_arg_regex) && !regex_match(arguments[i + 1], long_arg_regex)) {
                value = arguments[++i];
            }
            args_map[key] = trim(value);
        } else if (regex_match(arg_t, match, combined_short_arg_regex)) {
            for (size_t j = 1; j < match.str(0).length(); ++j) {
                args_map[string(1, match.str(0)[j])] = "";
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

std::optional<std::string> args::get(const std::string& key) const noexcept {
    auto it = args_map.find(key);
    if (it != args_map.end()) {
        return it->second;
    }
    return std::nullopt;
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
std::string args::to_str(const args& a) noexcept {
    if (a.args_map.empty()) {
        return "";
    }
    std::stringstream ss{};
    json_helper jh{};
    ss << jh.from(a.args_map);
    return ss.str();
}
