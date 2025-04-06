#include <MazeBuilder/json_helper.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <filesystem>
#include <stdexcept>

using namespace mazes;

class json_helper::json_helper_impl {
public:
    /// @brief Transform a map into a JSON string
    /// @param m
    /// @param pretty_print 
    /// @return the string in JSON format 
    std::string dump_s(const std::unordered_map<std::string, std::string>& m, int pretty_print = 4) const noexcept {
        nlohmann::json my_json{ m };
        return my_json.dump(pretty_print);
    }

    /// @brief Parse a JSON string into a map
    /// @param s 
    /// @param m 
    /// @return success or failure parse
    bool from(const std::string& s, std::unordered_map<std::string, std::string>& m) const noexcept {
        try {
            nlohmann::json j = nlohmann::json::parse(s);

            for (auto jit = j.cbegin(); jit != j.cend(); ++jit) {
                m[jit.key()] = jit.value().dump();
            }

            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }

    bool load(const std::string& filename, std::unordered_map<std::string, std::string>& m) const noexcept {
        std::filesystem::path fp{ filename };
        if (!std::filesystem::exists(fp)) {
            return false;
        }

        std::ifstream ifs{ filename };
        if (!ifs.is_open()) {
            return false;
        }

        std::string s{ std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() };
        return this->from(s, m);
    }
};

json_helper::json_helper() : impl{ std::make_unique<json_helper_impl>() } {
}

json_helper::~json_helper() = default;

json_helper::json_helper(const json_helper& other) : impl(std::make_unique<json_helper_impl>(*other.impl)) {

}

json_helper& json_helper::operator=(const json_helper& other) {
    if (this == &other) {
        return *this;
    }
    impl = std::make_unique<json_helper_impl>(*other.impl);
    return *this;
}

/// @brief 
/// @param map 
/// @param pretty_print 4
/// @return 
std::string json_helper::from(const std::unordered_map<std::string, std::string>& map, int pretty_print) const noexcept {
    return this->impl->dump_s(std::cref(map), pretty_print);
}

bool json_helper::from(const std::string& s, std::unordered_map<std::string, std::string>& m) const noexcept {
    return this->impl->from(std::cref(s), std::ref(m));
}

bool json_helper::load(const std::string& filename, std::unordered_map<std::string, std::string>& m) const noexcept {
    return this->impl->load(std::cref(filename), std::ref(m));
}
