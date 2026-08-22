#include "game_state.hpp"

namespace amazing::game
{
    app_state game_state::current() const noexcept
    {
        return state_;
    }

    bool game_state::is_menu() const noexcept
    {
        return state_ == app_state::menu;
    }

    bool game_state::is_playing() const noexcept
    {
        return state_ == app_state::playing;
    }

    bool game_state::is_transition() const noexcept
    {
        return state_ == app_state::transition;
    }

    void game_state::show_menu() noexcept
    {
        state_ = app_state::menu;
        transition_seconds_remaining_ = 0.0f;
    }

    void game_state::start_playing() noexcept
    {
        state_ = app_state::playing;
        transition_seconds_remaining_ = 0.0f;
    }

    void game_state::begin_transition(const float seconds) noexcept
    {
        state_ = app_state::transition;
        transition_seconds_remaining_ = seconds > 0.0f ? seconds : 0.0f;
    }

    bool game_state::tick(const float dt_seconds) noexcept
    {
        if (state_ != app_state::transition)
        {
            return false;
        }

        transition_seconds_remaining_ -= dt_seconds;
        return transition_seconds_remaining_ <= 0.0f;
    }
}
