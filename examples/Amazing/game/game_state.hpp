#pragma once

namespace amazing::game
{
    enum class app_state
    {
        menu,
        playing,
        transition
    };

    class game_state
    {
    public:
        [[nodiscard]] app_state current() const noexcept;
        [[nodiscard]] bool is_menu() const noexcept;
        [[nodiscard]] bool is_playing() const noexcept;
        [[nodiscard]] bool is_transition() const noexcept;

        void show_menu() noexcept;
        void start_playing() noexcept;
        void begin_transition(float seconds) noexcept;

        [[nodiscard]] bool tick(float dt_seconds) noexcept;

    private:
        app_state state_{ app_state::menu };
        float transition_seconds_remaining_{ 0.0f };
    };
}
