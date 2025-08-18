#ifndef STATE_HPP
#define STATE_HPP

enum class State {
    // Physics is starting, show welcome screen
    SPLASH,
    // Main menu / configurations 
    OPTIONS,
    // Physics is running
    PLAY,
    // Level is generated but Physics is paused/options
    PAUSE,
    // Physics is exiting and done
    DONE,
    // Useful when knowing when to re-draw in Physics loop
    // Level is being generated and not yet playable
    UPLOADING_LEVEL,
};

#endif // STATE_HPP
