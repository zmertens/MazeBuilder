#ifndef STATE_HPP
#define STATE_HPP

enum class State {
    // Shows info about the game
    ABOUT_MENU,

    // Main navigation menu
    MAIN_MENU,

    // Connect to multiplayer server
    NET_CONNECT,

    // Select an alias
    PICK_ALIAS_MENU,

    // Single player mode
    PLAY_SINGLE_MODE,

    // Multiplayer mode
    PLAY_MULTI_MODE,

    // Game is paused
    PLAY_PAUSED,

    // Show high scores
    SCORES_MENU,

    // Game is starting, show welcome screen
    SPLASH,

    // Application is done and exiting
    DONE,
};

#endif // STATE_HPP
