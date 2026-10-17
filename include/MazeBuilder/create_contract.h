#ifndef CREATE_CONTRACT_H
#define CREATE_CONTRACT_H

#include <any>
#include <optional>
#include <type_traits>
#include <utility>

/// @file create_contract.h
/// @namespace mazes
namespace mazes
{
    class args;
    class randomizer;

    /// @brief Contract for the create function in the runtime
    /// @details Implementations return their result type-erased in a std::any. Results must be owning
    /// values (e.g. std::string, never std::string_view) and an empty std::any signals failure.
    /// Callers should use create_as<T>() to retrieve the result with a type check.
    class create_contract
    {
    public:
        virtual ~create_contract() = default;

        [[nodiscard]] virtual std::any create(const args &a, randomizer &rng) noexcept = 0;

        /// @brief Calls create and extracts the result as T
        /// @return The result, or std::nullopt if creation failed or produced a different type
        template <typename T>
        [[nodiscard]] std::optional<T> create_as(const args &a, randomizer &rng) noexcept
        {
            static_assert(std::is_same_v<T, std::decay_t<T>>, "T must be a plain value type");
            static_assert(std::is_copy_constructible_v<T>, "std::any requires copy-constructible results");

            auto result = create(a, rng);
            if (auto *value = std::any_cast<T>(&result))
            {
                return std::move(*value);
            }
            return std::nullopt;
        }
    };
}

#endif // CREATE_CONTRACT_H
