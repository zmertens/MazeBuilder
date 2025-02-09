#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/maze.h>

#include <nlohmann/json.hpp>

#include <MazeBuilder/maze_builder.h>

using namespace mazes;

class json_helper::json_helper_impl {
public:
    nlohmann::json my_json;

    json_helper_impl() = default;
    ~json_helper_impl() = default;
};

json_helper::json_helper() : impl{ std::make_unique<json_helper_impl>() } {
}

/// @brief Convert a JSON string to an object
/// @param m the maze object
/// @param json_str
/// @param pretty_spaces 4
void json_helper::to_json_s(const std::unique_ptr<maze>& m, std::string& json_str, unsigned int pretty_spaces) noexcept {
    nlohmann::json my_json;
    //my_json["num_cols"] = std::to_string(m->columns);
    //my_json["num_rows"] = std::to_string(m->rows);
    //my_json["height"] = std::to_string(m->height);
    //my_json["algo"] = factory::to_string(m->algo);
    //my_json["seed"] = std::to_string(m->seed);
    my_json["v"] = mazes::VERSION;
    this->impl->my_json = my_json;
    json_str = my_json.dump(pretty_spaces);
}
/// @brief Convert an object to a JSON string
/// @param obj 
/// @return std::string 
std::string json_helper::dump_s() const noexcept {
    return this->impl->my_json.dump();
}
