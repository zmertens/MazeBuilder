#ifndef PROCESSED_TEXT_H
#define PROCESSED_TEXT_H

#include <array>
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
        using artifacts = std::variant<std::monostate, std::string, std::string_view, char *>;

        static constexpr auto DIRTY_INDEX = 0, PROCESSED_INDEX = 1, BUFFER_SIZE = 2;

    public:
        /// @brief Gets the dirty (unprocessed) artifact and the processed artifact
        /// @return The dirty artifact
        [[nodiscard]] artifacts get_dirty() const noexcept { return doubled_buffer.at(DIRTY_INDEX); }

        /// @brief Gets the processed artifact
        /// @return The processed artifact
        [[nodiscard]] artifacts get_processed() const noexcept { return doubled_buffer.at(PROCESSED_INDEX); }

        /// @brief Sets the dirty (unprocessed) artifact and the processed artifact
        /// @param anything
        void set_dirty(artifacts anything) noexcept { doubled_buffer.at(DIRTY_INDEX) = std::move(anything); }

        /// @brief Sets the processed artifact
        /// @param anything The artifact to set as processed
        void set_processed(artifacts anything) noexcept { doubled_buffer.at(PROCESSED_INDEX) = std::move(anything); }

        /// @brief Required by resource_management::load(id, string_view).
        ///        Stores sv as the dirty (unprocessed) value; returns true on success.
        bool set(std::string_view sv) noexcept
        {
            set_dirty(std::string{sv});
            return true;
        }

        /// @brief Checks if the dirty artifact is clean - empty or null
        /// @details clean artifacts are considered non-empty or don't care
        /// @return True if the dirty artifact is clean, false otherwise
        [[nodiscard]] constexpr bool is_clean() const noexcept
        {
            return std::visit([](const auto &value) -> bool
                              {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::monostate>)
                {
                    return true; // Consider monostate as clean
                }
                else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
                {
                    return value.empty();
                }
                else if constexpr (std::is_same_v<T, char*>)
                {
                    return value == nullptr || *value == '\0';
                }
                else
                {
                    return false;
                } }, get_dirty());
        }

    private:
        std::array<artifacts, BUFFER_SIZE> doubled_buffer;
    };
}

#endif // PROCESSED_TEXT_H
