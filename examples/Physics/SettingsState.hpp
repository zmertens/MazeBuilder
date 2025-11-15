#ifndef SETTINGS_STATE_HPP
#define SETTINGS_STATE_HPP

#include "Sprite.hpp"
#include "State.hpp"

class SettingsState : public State
{
public:
    explicit SettingsState(StateStack& stack, Context context);

    void draw() const noexcept override;
    bool update(float dt, unsigned int subSteps) noexcept override;
    bool handleEvent(const SDL_Event& event) noexcept override;

private:
    Sprite mBackgroundSprite;

    bool mShowText;

    // Settings UI state variables
    mutable bool mShowSettingsWindow;
};

#endif // SETTINGS_STATE_HPP

