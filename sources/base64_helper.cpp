#include <MazeBuilder/base64_helper.h>

#include <cpp-base64/base64.h>

using namespace mazes;

std::string base64_helper::encode(std::string_view sv) noexcept
{

    return base64_encode(std::string{sv});
}

std::string base64_helper::decode(std::string_view sv) noexcept
{

    return base64_decode(std::string{sv});
}
