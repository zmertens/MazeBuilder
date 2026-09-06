#pragma once

#include <SFML/System/Vector2.hpp>

namespace amazing::game
{
    class player_controller
    {
    public:
        void reset() noexcept;
        void on_left_click() noexcept;
        void set_move_direction(sf::Vector2f direction) noexcept;

        [[nodiscard]] sf::Vector2f facing_direction() const noexcept;
        [[nodiscard]] float rotation_degrees() const noexcept;
        [[nodiscard]] bool is_moving() const noexcept;

    private:
        sf::Vector2f facing_direction_{ 0.0f, -1.0f };
        bool moving_{ false };
    };
}
