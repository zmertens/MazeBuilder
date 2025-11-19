#ifndef STATE_IDENTIFIERS_HPP
#define STATE_IDENTIFIERS_HPP


namespace States
{
    enum class ID : unsigned int
    {
        DONE = 0,
        GAME = 1,
        LOADING = 2,
        MENU = 3,
        PAUSE = 4,
        RESETTING = 5,
        SETTINGS = 6,
        SPLASH = 7,
    };
}

#endif // STATE_IDENTIFIERS_HPP
