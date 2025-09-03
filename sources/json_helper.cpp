#include <MazeBuilder/json_helper.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <filesystem>
#include <stdexcept>

using namespace mazes;

class json_helper::json_helper_impl
{
public:
    /// @brief Transform a map into a JSON string
    /// @param m
    /// @param pretty_print
    /// @return the string in JSON format
    std::string dump_s(const std::unordered_map<std::string, std::string> &m, int pretty_print = 4) const noexcept
    {

        nlohmann::json my_json{m};

        return my_json.dump(pretty_print);
    }

    std::string dump_s(const std::vector<std::unordered_map<std::string, std::string>> &arr, int pretty_print = 4) const noexcept
    {

        nlohmann::json array_json = nlohmann::json::array();

        for (const auto &config_map : arr)
        {

            nlohmann::json obj;
            for (const auto &[key, value_str] : config_map)
            {

                try
                {

                    // Try to parse the value as JSON
                    obj[key] = nlohmann::json::parse(value_str);
                }
                catch (...)
                {

                    // If parsing fails, store as string
                    obj[key] = value_str;
                }
            }
            array_json.push_back(obj);
        }

        return array_json.dump(pretty_print);
    }

    /// @brief Parse a JSON string into a map
    /// @param s
    /// @param m
    /// @return success or failure parse
    bool from(const std::string &s, std::unordered_map<std::string, std::string> &m) const noexcept
    {
        try
        {
            nlohmann::json j = nlohmann::json::parse(s);

            for (auto jit = j.cbegin(); jit != j.cend(); ++jit)
            {
                m[jit.key()] = jit.value().dump();
            }

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool load(const std::string &filename, std::unordered_map<std::string, std::string> &m) const noexcept
    {
        std::filesystem::path fp{filename};
        if (!std::filesystem::exists(fp))
        {
            return false;
        }

        std::ifstream ifs{filename};
        if (!ifs.is_open())
        {
            return false;
        }

        std::string s{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
        return this->from(s, m);
    }

    /// @brief Parse a JSON array string into a vector of maps
    /// @param s JSON string containing an array of objects
    /// @param vm Vector of maps to populate with parsed objects
    /// @return success or failure on parse
    bool from_array(const std::string &s, std::vector<std::unordered_map<std::string, std::string>> &vm) const noexcept
    {
        try
        {
            nlohmann::json j = nlohmann::json::parse(s);

            if (!j.is_array())
            {
                // If not an array, try to parse as a single object and add to vector
                std::unordered_map<std::string, std::string> single_obj;
                if (from(s, single_obj))
                {
                    vm.push_back(std::move(single_obj));
                    return true;
                }
                return false;
            }

            vm.reserve(j.size());

            for (const auto &obj : j)
            {
                std::unordered_map<std::string, std::string> item_map;
                for (auto it = obj.cbegin(); it != obj.cend(); ++it)
                {
                    item_map[it.key()] = it.value().dump();
                }
                vm.push_back(std::move(item_map));
            }

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    /// @brief Load a JSON array file into a vector of maps
    /// @param filename Path to JSON file containing an array of objects
    /// @param vm Vector of maps to populate with parsed objects
    /// @return success or failure on load/parse
    bool load_array(const std::string &filename, std::vector<std::unordered_map<std::string, std::string>> &vm) const noexcept
    {
        std::filesystem::path fp{filename};
        if (!std::filesystem::exists(fp))
        {
            return false;
        }

        std::ifstream ifs{filename};
        if (!ifs.is_open())
        {
            return false;
        }

        std::string s{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
        return this->from_array(s, vm);
    }
};

json_helper::json_helper() : impl{std::make_unique<json_helper_impl>()}
{
}

json_helper::~json_helper() = default;

json_helper::json_helper(const json_helper &other) : impl(std::make_unique<json_helper_impl>(*other.impl))
{
}

json_helper &json_helper::operator=(const json_helper &other)
{
    if (this == &other)
    {
        return *this;
    }
    impl = std::make_unique<json_helper_impl>(*other.impl);
    return *this;
}

/// @brief Dump a map to a JSON string
/// @param map
/// @param pretty_print 4
/// @return
std::string json_helper::from(const std::unordered_map<std::string, std::string> &map, int pretty_print) const noexcept
{

    return this->impl->dump_s(std::cref(map), pretty_print);
}

/// @brief Dump a vector of maps to a JSON string
/// @param arr
/// @param pretty_print 4
/// @return
std::string json_helper::from(const std::vector<std::unordered_map<std::string, std::string>> &arr, int pretty_print) const noexcept
{

    return this->impl->dump_s(std::cref(arr), pretty_print);
}

bool json_helper::from(const std::string &s, std::unordered_map<std::string, std::string> &m) const noexcept
{

    return this->impl->from(std::cref(s), std::ref(m));
}

bool json_helper::load(const std::string &filename, std::unordered_map<std::string, std::string> &m) const noexcept
{

    return this->impl->load(std::cref(filename), std::ref(m));
}

bool json_helper::from_array(const std::string &s, std::vector<std::unordered_map<std::string, std::string>> &vm) const noexcept
{

    return this->impl->from_array(std::cref(s), std::ref(vm));
}

bool json_helper::load_array(const std::string &filename, std::vector<std::unordered_map<std::string, std::string>> &vm) const noexcept
{

    return this->impl->load_array(std::cref(filename), std::ref(vm));
}
