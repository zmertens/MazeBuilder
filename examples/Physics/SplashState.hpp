#ifndef SPLASH_STATE_HPP
#define SPLASH_STATE_HPP

#include "Sprite.hpp"
#include "State.hpp"

class SplashState : public State {
public:

    explicit SplashState(StateStack& stack, Context context);

    virtual void draw() const noexcept override;
    virtual bool update(float dt) noexcept override;
    virtual bool handleEvent(const SDL_Event& event) noexcept override;

private:
    Sprite mSplashSprite;

    bool mShowText;
};

#endif // SPLASH_STATE_HPP
