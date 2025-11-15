#ifndef SPLASH_STATE_HPP
#define SPLASH_STATE_HPP

#include "Sprite.hpp"
#include "State.hpp"

class SplashState : public State
{
public:
    explicit SplashState(StateStack& stack, Context context);

    void draw() const noexcept override;
    bool update(float dt, unsigned int subSteps) noexcept override;
    bool handleEvent(const SDL_Event& event) noexcept override;

private:
    bool isLoadingComplete() const noexcept;

    Sprite mSplashSprite;

    bool mShowText;
};

#endif // SPLASH_STATE_HPP
