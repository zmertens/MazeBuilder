#include <MazeBuilder/base64_helper.h>

#include <cpp-base64/base64.h>

using namespace mazes;

class base_64_helper::base_64_helper_impl {
public:
    base_64_helper_impl() = default;
    ~base_64_helper_impl() = default;
    void encode(const std::string& encode, std::string& result) const noexcept {
        result = base64_encode(encode);
    }
    void decode(const std::string& decode, std::string& result) const noexcept {
        result = base64_decode(decode);
    }
};

base_64_helper::base_64_helper() : impl{ std::make_unique<base_64_helper_impl>() } {}

void base_64_helper::encode(const std::string& encode, std::string& result) const noexcept {
    this->impl->encode(encode, result);
}


void base_64_helper::decode(const std::string& decode, std::string& result) const noexcept {
    this->impl->decode(decode, result);
}
