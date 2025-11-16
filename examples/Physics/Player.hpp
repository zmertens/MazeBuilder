#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <cstdint>
#include <map>

#include "Command.hpp"

union SDL_Event;

class CommandQueue;
class Entity;

class Player
{
public:
    enum class Action
    {
        MOVE_LEFT,
        MOVE_RIGHT,
        JUMP,
        ACTION_COUNT
    };

    explicit Player();

    void handleEvent(const SDL_Event& event, CommandQueue& commands);
    void handleRealtimeInput(CommandQueue& commands);

    void assignKey(Action action, std::uint32_t key);
    [[nodiscard]] std::uint32_t getAssignedKey(Action action) const;

    void onBeginContact(Entity* other) noexcept;
    void onEndContact(Entity* other) noexcept;

    void setGroundContact(bool contact);
    bool hasGroundContact() const;

    bool isActive() const noexcept;
    void setActive(bool active) noexcept;

private:
    void initializeActions();
    static bool isRealtimeAction(Action action);

    std::map<std::uint32_t, Action> mKeyBinding;
    std::map<Action, Command> mActionBinding;
    bool mIsActive;
    bool mIsOnGround;
};

#endif // PLAYER_HPP
