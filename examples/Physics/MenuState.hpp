#ifndef MENU_STATE_HPP
#define MENU_STATE_HPP

#include "Sprite.hpp"
#include "State.hpp"

class MenuState : public State {
public:

    explicit MenuState(StateStack& stack, Context context);

    void draw() const noexcept override;
    bool update(float dt) noexcept override;
    bool handleEvent(const SDL_Event& event) noexcept override;

private:
    Sprite mBackgroundSprite;

    bool mShowText;
};

#endif // MENU_STATE_HPP
