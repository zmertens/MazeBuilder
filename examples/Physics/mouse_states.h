#ifndef MOUSE_STATES_H
#define MOUSE_STATES_H

struct mouse_states
{
    enum class button_state
    {
        UP,
        DOWN,
        PRESSED,
        RELEASED
    };

    button_state leftButton = button_state::UP;

    button_state rightButton = button_state::UP;

    int x = 0;
    int y = 0;
};

#endif MOUSE_STATES_H
