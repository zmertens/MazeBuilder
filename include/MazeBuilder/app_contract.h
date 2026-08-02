#ifndef APP_CONTRACT_H
#define APP_CONTRACT_H

#include <string_view>

/// @file app_contract.h
/// @namespace mazes
namespace mazes
{
    /// @brief Class for enforcing contracts on the runtime
    class app_contract
    {
    public:
        virtual ~app_contract() = default;

        /// @brief Apply the contract to the given unformatted string view
        /// @param unformatted_sv The unformatted string view to apply the contract to
        /// @return The formatted string view after applying the contract
        [[nodiscard]] virtual std::string_view apply(std::string_view unformatted_sv) noexcept = 0;
    };
} // namespace mazes

#endif // APP_CONTRACT_H
