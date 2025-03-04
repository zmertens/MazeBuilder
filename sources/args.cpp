#include <MazeBuilder/args.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/writer.h>

#include <string>
#include <regex>
#include <functional>
#include <algorithm>
#include <numeric>
#include <cctype>

using namespace mazes;

bool args::parse(const std::vector<std::string>& arguments) noexcept {
    using namespace std;

    if (arguments.empty()) {
        return false;
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
        return false;
    }

    // Helper method to remove whitespaces on left and right hand side of string
  
    auto trim = [](const std::string& str) -> std::string {
        auto front = std::find_if_not(str.begin(), str.end(), [](int c) { return std::isspace(c); });
        auto back = std::find_if_not(str.rbegin(), str.rend(), [](int c) { return std::isspace(c); }).base();
        return (back <= front ? std::string() : std::string(front, back));
        };

    auto args_trimmed = trim(cref(arguments));

    // Check if the arguments are valid by pattern matching
    const regex pattern{ ArgsPattern };
    if (!regex_match(args_trimmed, pattern)) {
        return false;
    }

    auto it = args_trimmed.cbegin();
    auto end = args_trimmed.cend();

    // Extract the app name
    auto next_space = find(it, end, ' ');
    args_map.insert_or_assign("app", string(it, next_space));
    it = next_space;

    while (it != end) {
        // Skip spaces
        while (it != end && *it == ' ') {
            ++it;
        }

        if (it == end) {
            break;
        }

        if (*it == '-') {
            auto key_start = ++it;
            if (key_start != end && *key_start == '-') {
                // Long option
                ++key_start;
                auto key_end = find(key_start, end, '=');
                string key(key_start, key_end);
                if (key_end != end) {
                    ++key_end;
                    auto value_end = find(key_end, end, ' ');
                    args_map.insert_or_assign(key, string(key_end, value_end));
                    it = value_end;
                } else {
                    auto value_start = find(key_start, end, ' ');
                    args_map.insert_or_assign(key, string(value_start, end));
                    it = end;
                }
            } else {
                // Short option
                auto key_end = find_if(key_start, end, [](char c) { return c == ' ' || c == '-'; });
                string key(key_start, key_end);
                if (key_end != end && *key_end == ' ') {
                    ++key_end;
                    auto value_end = find(key_end, end, ' ');
                    args_map.insert_or_assign(key, string(key_end, value_end));
                    it = value_end;
                } else {
                    args_map.insert_or_assign(key, "");
                    it = key_end;
                }
            }
        } else {
            ++it;
        }
    }

    // Check if it's a JSON string or file
    //auto val = args_map.find("j");
    //if (val != args_map.cend()) {
    //    auto desc = val->second;
    //    // JSON file
    //    if (writer::is_file_with_suffix(cref(desc), output::JSON)) {
    //        json_helper jh{};
    //        return jh.load(cref(desc), ref(args_map));
    //    }
    //    // JSON string
    //    json_helper jh{};
    //    return jh.from(cref(desc), ref(args_map));
    //}
    //val = args_map.find("json");

    return true;
}

void args::clear() noexcept {
    args_map.erase(args_map.begin(), args_map.end());
}

std::string args::get_desc(const std::string& key) const noexcept {
    auto it = args_map.find(key);
    if (it != args_map.end()) {
        return it->second;
    }
    return "";
}

const std::unordered_map<std::string, std::string>& args::get_map() const noexcept {
    return args_map;
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
