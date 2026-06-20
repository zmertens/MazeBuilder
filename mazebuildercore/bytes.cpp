#include <MazeBuilder/bytes.h>

#include <cpp-base64/base64.h>

#include <utility>

using namespace mazes;

std::string bytes::encode(std::string_view sv) noexcept
{
    return base64_encode(std::string{sv});
}

std::string bytes::decode(std::string_view sv) noexcept
{
    return base64_decode(std::string{sv});
}

std::string_view bytes::bytes_to_string(const std::vector<std::uint8_t>& bytes) noexcept
{
    if (bytes.empty())
    {
        return {};
    }

    // Construct string directly from bytes (binary-safe)
    return std::string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

std::vector<std::uint8_t> bytes::string_to_bytes(const std::string_view sv) noexcept
{
    if (sv.empty())
    {
        return {};
    }

    return std::vector(reinterpret_cast<const std::uint8_t*>(sv.data()),
                       reinterpret_cast<const std::uint8_t*>(sv.data()) + sv.size());
}

std::string bytes::to_base36(std::uint32_t value) noexcept
{

    static constexpr auto base36_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    std::string result;

    do
    {

        result.push_back(base36_chars[value % 36]);

        value /= 36;
    } while (value > 0);

    std::reverse(result.begin(), result.end());

    return result;
}
