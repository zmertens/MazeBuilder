#ifndef PROCESSED_TEXT_H
#define PROCESSED_TEXT_H

#include <type_traits>
#include <variant>

/// @namespace mazes
/// @file processed_text.h
namespace mazes
{
    /// @brief Class for managing processed text resources
    class processed_text final
    {
        /// @brief Type alias for the artifacts stored in the double buffer
        using artifacts = std::variant<std::monostate, std::string, std::string_view, char*>;

    public:
        /// @brief Gets the processed artifact
        /// @return The processed artifact
        [[nodiscard]] artifacts get() const noexcept { return buffer; }

        /// @brief Sets the processed artifact
        /// @param anything The artifact to set as processed
        void set_processed(artifacts anything) noexcept { buffer = std::move(anything); }

        /// @brief Sets the processed artifact from a string view
        /// @param sv The string view to set as processed
        /// @return True on success
        bool set(const std::string_view sv) noexcept
        {
            set_processed(std::string{sv});
            return true;
        }

        /// @brief Checks if the processed artifact is finalized - empty or null
        /// @details finalized artifacts are considered non-empty or don't care
        /// @return True if the processed artifact is finalized, false otherwise
        [[nodiscard]] bool is_processed() const noexcept
        {
            return std::visit([](const auto& value) -> bool
            {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::monostate>)
                {
                    return false;
                }
                else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
                {
                    return !value.empty();
                }
                else if constexpr (std::is_same_v<T, char*>)
                {
                    return value != nullptr && *value != '\0';
                }
                else
                {
                    return false;
                }
            }, get());
        }

        static std::string to_string(const artifacts& artifact) noexcept
        {
            return std::visit([](const auto& value) -> std::string
            {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::monostate>)
                {
                    return {};
                }
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    return value;
                }
                else if constexpr (std::is_same_v<T, std::string_view>)
                {
                    return std::string{value};
                }
                else if constexpr (std::is_same_v<T, char*>)
                {
                    return value ? std::string{value} : std::string{};
                }
                else
                {
                    return {};
                }
            }, artifact);
        }

    private:
        artifacts buffer;
    };
}

#endif // PROCESSED_TEXT_H
