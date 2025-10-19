#ifndef STATE_HPP
#define STATE_HPP

enum class State {
    ABOUT_MENU,
    MAIN_MENU,
    NET_CONNECT,
    PICK_ALIAS_MENU,
    PLAY_SINGLE_MODE,
    PLAY_MULTI_MODE,
    PLAY_PAUSED,
    SCORES_MENU,
    // Physics is starting, show welcome screen
    SPLASH,
    // Application is exiting
    DONE,
};

#endif // STATE_HPP
