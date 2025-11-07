#include "MenuState.hpp"

#include <SDL3/SDL.h>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>

#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "StateStack.hpp"
#include "backends/imgui_impl_sdlrenderer3.h"

MenuState::MenuState(StateStack& stack, Context context)
    : State(stack, context)
      , mShowText{true}
      , mBackgroundSprite{context.textures->get(Textures::ID::SDL_BLOCKS)}
{

}

void MenuState::draw() const noexcept
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    static auto showDemoWindow{true};
    if (showDemoWindow) {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), getContext().window->getRenderer());

    auto& window = *getContext().window;

    window.draw(mBackgroundSprite);
}

bool MenuState::update(float dt) noexcept
{

    return true;
}

bool MenuState::handleEvent(const SDL_Event& event) noexcept
{
    ImGui_ImplSDL3_ProcessEvent(&event);

    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.scancode == SDL_SCANCODE_ESCAPE)
        {
            SDL_Log("MenuState: Escape Key pressed, returning to previous state");

            // Pop the current state
            requestStackPop();
            // Push the game state
            requestStackPush(States::ID::GAME);
            mShowText = !mShowText;
        }
    }

    return true;
}
