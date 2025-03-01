#include <MazeBuilder/json_helper.h>

#include <nlohmann/json.hpp>

using namespace mazes;

class json_helper::json_helper_impl {
public:
    /// @brief 
    /// @param m
    /// @param pretty_print 
    /// @return 
    std::string dump_s(const std::unordered_map<std::string, std::string>& m, int pretty_print = 4) const noexcept {
        nlohmann::json my_json{ m };
        return my_json.dump(pretty_print);
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

