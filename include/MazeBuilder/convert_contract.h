#ifndef CONVERT_CONTRACT_H
#define CONVERT_CONTRACT_H

#include <optional>
#include <string_view>

namespace mazes
{
    class args;

    /// @brief Contract for converting arguments to args
    class convert_contract
    {
    public:
        virtual ~convert_contract() = default;
        /// @brief Convert arguments to args
        /// @param arguments The arguments to convert
        /// @return The converted args, or std::nullopt if conversion failed
        [[nodiscard]] virtual std::optional<args> convert(std::string_view arguments) const noexcept = 0;
    };

    /// @brief Convenience alias used by states that implement argument parsing
    using convert = convert_contract;
}

#endif // CONVERT_CONTRACT_H
