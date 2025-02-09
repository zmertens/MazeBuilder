#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include <string>
#include <memory>

namespace mazes {

    class maze;

    class json_helper {
    public:
        explicit json_helper();

        void to_json_s(const std::unique_ptr<maze>& m, std::string& str, unsigned int pretty_spaces = 4) noexcept;
        std::string dump_s() const noexcept;

    private:
        class json_helper_impl;
        std::unique_ptr<json_helper_impl> impl;
    }; // class

} // namespace

#endif // JSON_HELPER_H
