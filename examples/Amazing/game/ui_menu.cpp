#include "ui_menu.hpp"

#include <array>

namespace amazing::game
{
    menu_action ui_menu::on_key_pressed(const sf::Keyboard::Key key) noexcept
    {
        constexpr std::size_t LAST_INDEX = 2u;

        if (key == sf::Keyboard::Key::Up)
        {
            selected_index_ = selected_index_ == 0u ? LAST_INDEX : selected_index_ - 1u;
            return menu_action::none;
        }

        if (key == sf::Keyboard::Key::Down)
        {
            selected_index_ = selected_index_ == LAST_INDEX ? 0u : selected_index_ + 1u;
            return menu_action::none;
        }

        if (key == sf::Keyboard::Key::Enter)
        {
            switch (selected_index_)
            {
            case 0u:
                return menu_action::start;
            case 1u:
                return menu_action::rebuild;
            case 2u:
                return menu_action::quit;
            default:
                break;
            }
        }

        return menu_action::none;
    }

    void ui_menu::draw(sf::RenderTarget& target, const sf::Font& font, const sf::Texture* background_texture) const
    {
        const auto size = target.getSize();

        if (background_texture)
        {
            sf::Sprite background{ *background_texture };
            const auto texture_size = background_texture->getSize();
            if (texture_size.x > 0u && texture_size.y > 0u)
            {
                background.setScale({ static_cast<float>(size.x) / static_cast<float>(texture_size.x),
                                     static_cast<float>(size.y) / static_cast<float>(texture_size.y) });
            }
            target.draw(background);
        }

        sf::Text title{ font, "Amazing Roguelike", 42u };
        title.setFillColor(sf::Color(245, 245, 235));
        title.setOutlineColor(sf::Color(15, 15, 15));
        title.setOutlineThickness(2.0f);
        title.setPosition({ 24.0f, 28.0f });
        target.draw(title);

        sf::Text controls{ font, "Up/Down: select\nEnter: confirm", 20u };
        controls.setFillColor(sf::Color(235, 235, 120));
        controls.setOutlineColor(sf::Color(15, 15, 15));
        controls.setOutlineThickness(1.4f);
        controls.setPosition({ 24.0f, 88.0f });
        target.draw(controls);

        for (std::size_t idx = 0u; idx < 3u; ++idx)
        {
            const std::string prefix = selected_index_ == idx ? "> " : "  ";
            sf::Text entry{ font, prefix + action_label(idx), 30u };
            entry.setFillColor(selected_index_ == idx ? sf::Color(255, 170, 100) : sf::Color(245, 245, 235));
            entry.setOutlineColor(sf::Color(15, 15, 15));
            entry.setOutlineThickness(1.6f);
            entry.setPosition({ 24.0f, 210.0f + static_cast<float>(idx) * 44.0f });
            target.draw(entry);
        }
    }

    std::string ui_menu::action_label(const std::size_t index)
    {
        constexpr std::array<const char*, 3> LABELS{ "Start", "Quit", "About"};
        return LABELS[index < LABELS.size() ? index : 0u];
    }
}
