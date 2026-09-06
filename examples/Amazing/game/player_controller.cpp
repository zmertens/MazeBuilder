#include "player_controller.hpp"

#include <array>
#include <cmath>

namespace amazing::game
{
    namespace
    {
        constexpr std::array<sf::Vector2f, 4> DIRECTIONS{ sf::Vector2f{ 0.0f, -1.0f }, sf::Vector2f{ -1.0f, 0.0f }, sf::Vector2f{ 0.0f, 1.0f }, sf::Vector2f{ 1.0f, 0.0f } };
    }

    void player_controller::reset() noexcept
    {
        facing_direction_ = { 0.0f, -1.0f };
        moving_ = false;
    }

    void player_controller::on_left_click() noexcept
    {
        int index = 0;
        for (std::size_t i = 0; i < DIRECTIONS.size(); ++i)
        {
            if (DIRECTIONS[i] == facing_direction_)
            {
                index = static_cast<int>(i);
                break;
            }
        }

        const int next_index = (index + 1) % static_cast<int>(DIRECTIONS.size());
        facing_direction_ = DIRECTIONS[static_cast<std::size_t>(next_index)];
        moving_ = true;
    }

    void player_controller::set_move_direction(const sf::Vector2f direction) noexcept
    {
        const float len_sq = direction.x * direction.x + direction.y * direction.y;
        if (len_sq <= 1e-6f)
        {
            moving_ = false;
            return;
        }

        const float inv_len = 1.0f / std::sqrt(len_sq);
        facing_direction_ = { direction.x * inv_len, direction.y * inv_len };
        moving_ = true;
    }

    sf::Vector2f player_controller::facing_direction() const noexcept
    {
        return facing_direction_;
    }

    float player_controller::rotation_degrees() const noexcept
    {
        constexpr float RAD_TO_DEG = 57.2957795f;
        const float angle = std::atan2(facing_direction_.y, facing_direction_.x) * RAD_TO_DEG + 90.0f;
        return angle;
    }

    bool player_controller::is_moving() const noexcept
    {
        return moving_;
    }
}
