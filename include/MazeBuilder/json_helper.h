#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include <string>
#include <memory>
#include <unordered_map>

namespace mazes {

    class json_helper {
    public:
        explicit json_helper();

        ~json_helper();

        // Copy constructor
        json_helper(const json_helper& other);

        // Copy assignment operator
        json_helper& operator=(const json_helper& other);

        // Move constructor
        json_helper(json_helper&& other) noexcept = default;

        // Move assignment operator
        json_helper& operator=(json_helper&& other) noexcept = default;

        /// @brief Get the contents as a C++ string of the JSON impl
        /// @param map 
        /// @param pretty_print 
        /// @return 
        std::string from(const std::unordered_map<std::string, std::string>& map, int pretty_print = 4) const noexcept;

    private:
        class json_helper_impl;
        std::unique_ptr<json_helper_impl> impl;
    };

} // namespace mazes

#endif // JSON_HELPER_H
