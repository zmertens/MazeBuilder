#include <MazeBuilder/base64_helper.h>

#include <cpp-base64/base64.h>

using namespace mazes;

/// @brief Implementation class for the base64 helper
class base64_helper::base64_helper_impl {
public:
    base64_helper_impl() = default;

    ~base64_helper_impl() = default;

    std::string encode(const std::string& s) const noexcept {
        return base64_encode(s);
    }
    
    std::string decode(const std::string& s) const noexcept {
        return base64_decode(s);
    }
};

base64_helper::base64_helper() : impl{ std::make_unique<base64_helper_impl>() } {}

base64_helper::~base64_helper() = default;

base64_helper::base64_helper(const base64_helper& other) : impl(std::make_unique<base64_helper_impl>(*other.impl)) {

}

std::string base64_helper::encode(const std::string& s) const noexcept {
    return this->impl->encode(std::cref(s));
}


std::string base64_helper::decode(const std::string& s) const noexcept {
    return this->impl->decode(std::cref(s));
}
