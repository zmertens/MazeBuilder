#include "player_controller.hpp"

#include <array>

namespace amazing::game
{
    namespace
    {
        constexpr std::array<sf::Vector2f, 4> DIRECTIONS{ sf::Vector2f{ 0.0f, -1.0f }, sf::Vector2f{ -1.0f, 0.0f }, sf::Vector2f{ 0.0f, 1.0f }, sf::Vector2f{ 1.0f, 0.0f } };
    }

    void player_controller::reset() noexcept
    {
        facing_index_ = 0;
    }

    void player_controller::on_left_click() noexcept
    {
        facing_index_ = (facing_index_ + 1) % static_cast<int>(DIRECTIONS.size());
    }

    sf::Vector2f player_controller::facing_direction() const noexcept
    {
        return DIRECTIONS[static_cast<std::size_t>(facing_index_)];
    }

    float player_controller::rotation_degrees() const noexcept
    {
        return static_cast<float>(facing_index_) * -90.0f;
    }

    bool player_controller::is_moving() const noexcept
    {
        return true;
    }
}
