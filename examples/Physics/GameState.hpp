#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include "State.hpp"
#include "World.hpp"

class Player;
class StateStack;
union SDL_Event;

class GameState : public State
{
public:
    explicit GameState(StateStack& stack, Context context);

    void draw() const noexcept override;
    bool update(float dt, unsigned int subSteps) noexcept override;
    bool handleEvent(const SDL_Event& event) noexcept override;

private:
    World mWorld;
    Player& mPlayer;
};

#endif // GAME_STATE_HPP
