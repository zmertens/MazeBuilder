#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <cstdint>
#include <map>

#include "Command.hpp"

union SDL_Event;

class CommandQueue;

class Player {
public:

    enum class Action {
        MOVE_LEFT,
        MOVE_RIGHT,
        MOVE_UP,
        MOVE_DOWN,
        ACTION_COUNT
    };

public:
    explicit Player();

    void handleEvent(const SDL_Event& event, CommandQueue& commands);
    void handleRealtimeInput(CommandQueue& commands);

    void assignKey(Action action, std::uint32_t key);
    std::uint32_t getAssignedKey(Action action) const;


private:
    void initializeActions();
    static bool isRealtimeAction(Action action);


private:
    std::map<std::uint32_t, Action> mKeyBinding;
    std::map<Action, Command> mActionBinding;
};

#endif // PLAYER_HPP
