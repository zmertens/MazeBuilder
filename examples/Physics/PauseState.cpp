#include "PauseState.hpp"

#include <SDL3/SDL.h>

#include <dearimgui/imgui.h>

#include "Font.hpp"
#include "MenuState.hpp"
#include "ResourceManager.hpp"

PauseState::PauseState(StateStack& stack, Context context)
    : State(stack, context)
      , mBackgroundShape{}, mSelectedMenuItem(static_cast<unsigned int>(States::ID::PAUSE))
{
}

void PauseState::draw() const noexcept
{
    ImGui::PushFont(getContext().fonts->get(Fonts::ID::NUNITO_SANS).get());

    // Apply color schema
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.016f, 0.047f, 0.024f, 0.95f)); // #040c06
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.067f, 0.137f, 0.094f, 1.0f)); // #112318
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.118f, 0.227f, 0.161f, 1.0f)); // #1e3a29
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.188f, 0.365f, 0.259f, 1.0f)); // #305d42
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.302f, 0.502f, 0.380f, 1.0f)); // #4d8061
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.537f, 0.635f, 0.341f, 1.0f)); // #89a257
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.302f, 0.502f, 0.380f, 1.0f)); // #4d8061
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.537f, 0.635f, 0.341f, 1.0f)); // #89a257
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.745f, 0.863f, 0.498f, 1.0f)); // #bedc7f
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.933f, 1.0f, 0.8f, 1.0f)); // #eeffcc

    static auto showThePauseStateWindow{true};
    if (ImGui::Begin("Pause Menu", &showThePauseStateWindow, ImGuiWindowFlags_NoCollapse))
    {

        ImGui::Text("Welcome to MazeBuilder Physics");
        ImGui::Separator();
        ImGui::Spacing();

        // Navigation options
        ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f), "Navigation Options:");
        ImGui::Spacing();

        if (ImGui::Button("Resume Game", ImVec2(200, 40)))
        {
            mSelectedMenuItem = static_cast<unsigned int>(States::ID::GAME);
            SDL_Log("Navigation: Resume Game selected");
        }
        ImGui::Spacing();

        if (ImGui::Button("Main Menu", ImVec2(200, 40)))
        {
            mSelectedMenuItem = static_cast<unsigned int>(States::ID::MENU);
            SDL_Log("Navigation: Main Menu selected");
        }
        ImGui::Spacing();

        if (ImGui::Button("Exit Game", ImVec2(200, 40)))
        {
            mSelectedMenuItem = static_cast<unsigned int>(States::ID::DONE);
            SDL_Log("Navigation: Exit Game selected");
        }
        ImGui::Spacing();
    }
    ImGui::End();
    ImGui::PopStyleColor(10);
    ImGui::PopFont();

    // auto& window = *getContext().window;
    // window.draw(mBackgroundShape);
}

bool PauseState::update(float dt, unsigned int subSteps) noexcept
{
    switch (mSelectedMenuItem)
    {
    case static_cast<unsigned int>(States::ID::DONE):
        requestStateClear();
        break;
    case static_cast<unsigned int>(States::ID::GAME):
        SDL_Log("PauseState: Resuming game...");
        requestStackPop();
        break;
    case static_cast<unsigned int>(States::ID::MENU):
        SDL_Log("Menu selected from PauseState, clearing states and returning to MenuState");
        // Pop pause state
        requestStackPop();
        // Pop game state
        requestStackPop();
        // Push menu state
        requestStackPush(States::ID::MENU);
        break;
    default: break;
    }

    return true;
}

bool PauseState::handleEvent(const SDL_Event& event) noexcept
{

    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.scancode == SDL_SCANCODE_ESCAPE)
        {
            SDL_Log("PauseState: Escape Key pressed, returning to previous state");

            mSelectedMenuItem = static_cast<unsigned int>(States::ID::GAME);
        }
    }

    return true;
}
