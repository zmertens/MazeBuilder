#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <string>

namespace amazing::game
{
    enum class menu_action
    {
        none,
        start,
        rebuild,
        quit
    };

    class ui_menu
    {
    public:
        [[nodiscard]] menu_action on_key_pressed(sf::Keyboard::Key key) noexcept;
        void draw(sf::RenderTarget& target, const sf::Font& font, const sf::Texture* background_texture) const;

    private:
        std::size_t selected_index_{ 0u };

        [[nodiscard]] static std::string action_label(std::size_t index);
    };
}
