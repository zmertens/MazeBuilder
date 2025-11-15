#ifndef PAUSE_STATE_HPP
#define PAUSE_STATE_HPP

#include <SDL3/SDL_rect.h>

#include "State.hpp"

class PauseState : public State
{
public:
    explicit PauseState(StateStack& stack, Context context);

    void draw() const noexcept override;
    bool update(float dt, unsigned int subSteps) noexcept override;
    bool handleEvent(const SDL_Event& event) noexcept override;

private:
    SDL_Rect mBackgroundShape;

    mutable unsigned int mSelectedMenuItem;
};

#endif // PAUSE_STATE_HPP
