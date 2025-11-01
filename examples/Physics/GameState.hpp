#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include "State.hpp"
#include "World.hpp"

class Player;
class StateStack;

class GameState : public State {
public:

    explicit GameState(StateStack& stack, State::Context context);

    virtual void draw() const noexcept override;
    virtual bool update(float dt) noexcept override;
    virtual bool handleEvent(const SDL_Event& event) noexcept override;

private:
    World mWorld;
    Player& mPlayer;
};

#endif // GAME_STATE_HPP
