#ifndef LOADING_STATE_HPP
#define LOADING_STATE_HPP

#include "Sprite.hpp"
#include "State.hpp"
#include "WorkerConcurrent.hpp"

class StateStack;
union SDL_Event;

class LoadingState : public State {
public:

    explicit LoadingState(StateStack& stack, State::Context context);

    virtual void draw() const noexcept override;
    virtual bool update(float dt) noexcept override;
    virtual bool handleEvent(const SDL_Event& event) noexcept override;

private:
    void setCompletion(float percent) noexcept;

    Sprite mLoadingSprite;

    WorkerConcurrent mForeman;
    
    bool mHasFinished;
};

#endif // LOADING_STATE_HPP
