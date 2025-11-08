#include "MenuState.hpp"

#include <SDL3/SDL.h>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>

#include "Font.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "StateStack.hpp"
#include "backends/imgui_impl_sdlrenderer3.h"

MenuState::MenuState(StateStack& stack, Context context)
    : State(stack, context)
      , mShowText{true}
      , mSelectedMenuItem(0), mShowMenuWindow(true), mBackgroundSprite{context.textures->get(Textures::ID::SDL_BLOCKS)}
{
}

void MenuState::draw() const noexcept
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::PushFont(getContext().fonts->get(Fonts::ID::NUNITO_SANS).get());

    static auto showDemoWindow{true};
    if (showDemoWindow) {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }

    // Custom navigation menu window with color schema
    if (mShowMenuWindow) {
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

        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Main Menu", &mShowMenuWindow, ImGuiWindowFlags_NoCollapse)) {
            ImGui::Text("Welcome to MazeBuilder Physics");
            ImGui::Separator();
            ImGui::Spacing();

            // Navigation options
            ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f), "Navigation Options:");
            ImGui::Spacing();

            if (ImGui::Selectable("Start Game", mSelectedMenuItem == 0)) {
                mSelectedMenuItem = 0;
                SDL_Log("Navigation: Start Game selected");
            }
            ImGui::Spacing();

            if (ImGui::Selectable("Settings", mSelectedMenuItem == 1)) {
                mSelectedMenuItem = 1;
                SDL_Log("Navigation: Settings selected");
            }
            ImGui::Spacing();

            if (ImGui::Selectable("Controls", mSelectedMenuItem == 2)) {
                mSelectedMenuItem = 2;
                SDL_Log("Navigation: Controls selected");
            }
            ImGui::Spacing();

            if (ImGui::Selectable("About", mSelectedMenuItem == 3)) {
                mSelectedMenuItem = 3;
                SDL_Log("Navigation: About selected");
            }
            ImGui::Spacing();

            if (ImGui::Selectable("Exit", mSelectedMenuItem == 4)) {
                mSelectedMenuItem = 4;
                SDL_Log("Navigation: Exit selected");
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Display selected menu info
            ImGui::TextColored(ImVec4(0.933f, 1.0f, 0.8f, 1.0f), "Selected: ");
            ImGui::SameLine();
            const char* menuItems[] = {"Start Game", "Settings", "Controls", "About", "Exit"};
            if (mSelectedMenuItem >= 0 && mSelectedMenuItem < 5) {
                ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f), "%s", menuItems[mSelectedMenuItem]);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Action buttons
            if (ImGui::Button("Confirm Selection", ImVec2(180, 40))) {
                SDL_Log("Confirmed selection: %d", mSelectedMenuItem);
                // Add logic to handle menu selection
            }

            ImGui::SameLine();

            if (ImGui::Button("Toggle Demo", ImVec2(180, 40))) {
                showDemoWindow = !showDemoWindow;
            }
        }
        ImGui::End();
        ImGui::PopFont();

        ImGui::PopStyleColor(10);
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), getContext().window->getRenderer());

    auto& window = *getContext().window;

    window.draw(mBackgroundSprite);
}

bool MenuState::update(float dt) noexcept
{
    if (!mShowMenuWindow)
    {
        requestStackPop();
        requestStackPush(States::ID::GAME);
    }

    return true;
}

bool MenuState::handleEvent(const SDL_Event& event) noexcept
{
    ImGui_ImplSDL3_ProcessEvent(&event);

    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.scancode == SDL_SCANCODE_ESCAPE)
        {

            mShowText = !mShowText;
        }
    }

    return true;
}
