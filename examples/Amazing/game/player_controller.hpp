#pragma once

#include <SFML/System/Vector2.hpp>

namespace amazing::game
{
    class player_controller
    {
    public:
        void reset() noexcept;
        void on_left_click() noexcept;

        [[nodiscard]] sf::Vector2f facing_direction() const noexcept;
        [[nodiscard]] float rotation_degrees() const noexcept;
        [[nodiscard]] bool is_moving() const noexcept;

    private:
        int facing_index_{ 0 };
    };
}
