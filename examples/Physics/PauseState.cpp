#include "PauseState.hpp"

#include <SDL3/SDL.h>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_opengl3.h>

#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "StateStack.hpp"

PauseState::PauseState(StateStack& stack, Context context)
    : State(stack, context)
    , mBackgroundShape{}
{
}

void PauseState::draw() const noexcept
{
    auto& window = *getContext().window;
}

bool PauseState::update(float dt) noexcept
{
    return true;
}

bool PauseState::handleEvent(const SDL_Event& event) noexcept
{
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.scancode == SDL_SCANCODE_ESCAPE)
        {
            SDL_Log("PauseState: Escape Key pressed, returning to previous state");

            // Pop the loading state underneath
            requestStackPop();
        }
    }

    return true;
}
