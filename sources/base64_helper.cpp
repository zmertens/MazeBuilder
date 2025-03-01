#include <MazeBuilder/base64_helper.h>

#include <cpp-base64/base64.h>

using namespace mazes;

/// @brief Implementation class for the base64 helper
class base_64_helper::base_64_helper_impl {
public:
    base_64_helper_impl() = default;

    ~base_64_helper_impl() = default;

    std::string encode(const std::string& s) const noexcept {
        return base64_encode(s);
    }
    
    std::string decode(const std::string& s) const noexcept {
        return base64_decode(s);
    }
};

base_64_helper::base_64_helper() : impl{ std::make_unique<base_64_helper_impl>() } {}

base_64_helper::~base_64_helper() = default;

base_64_helper::base_64_helper(const base_64_helper& other) : impl(std::make_unique<base_64_helper_impl>(*other.impl)) {

}

std::string base_64_helper::encode(const std::string& s) const noexcept {
    return this->impl->encode(std::cref(s));
}


std::string base_64_helper::decode(const std::string& s) const noexcept {
    return this->impl->decode(std::cref(s));
}
