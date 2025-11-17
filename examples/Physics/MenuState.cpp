#include "MenuState.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <string>

#include <dearimgui/imgui.h>

#include "Font.hpp"
#include "PauseState.hpp"
#include "Player.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "StateStack.hpp"

MenuState::MenuState(StateStack& stack, Context context)
    : State(stack, context)
      , mBackgroundSprite{context.textures->get(Textures::ID::SPLASH_TITLE_IMAGE)}
      , mSelectedMenuItem(MenuItem::NEW_GAME)
      , mShowMainMenu(true), mItemSelectedFlags{}
{
    // initialize selection flags so UI shows correct selected item
    mItemSelectedFlags.fill(false);
    mItemSelectedFlags[static_cast<size_t>(mSelectedMenuItem)] = true;
}

void MenuState::draw() const noexcept
{
    if (!mShowMainMenu) {

        return;
    }

    using std::array;
    using std::size_t;
    using std::string;

    ImGui::PushFont(getContext().fonts->get(Fonts::ID::NUNITO_SANS).get());

    static auto showDemoWindow{false};
#if defined(MAZE_DEBUG)

    if (showDemoWindow) {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
#endif

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

    if (ImGui::Begin("Main Menu", &mShowMainMenu, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Welcome to MazeBuilder Physics");
        ImGui::Separator();
        ImGui::Spacing();

        // Navigation options
        ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f), "Navigation Options:");
        ImGui::Spacing();

        const array<string, static_cast<size_t>(MenuItem::COUNT)> menuItems = {
            "Resume", "New Game", "Settings", "Splash screen", "Quit"
        };

        // Use Selectable with bool* overload so ImGui keeps a consistent toggled state
        const auto active = static_cast<size_t>(getContext().player->isActive());
        for (size_t i{static_cast<size_t>(active ? 0 : 1)}; i < menuItems.size(); ++i)
        {
            if (bool* flag = &mItemSelectedFlags[i]; ImGui::Selectable(menuItems[i].c_str(), flag))
            {
                // When an item is selected, clear others and set the selected index
                for (auto& f : mItemSelectedFlags)
                {
                    f = false;
                }
                *flag = true;
                mSelectedMenuItem = static_cast<MenuItem>(i);
                SDL_Log("Navigation: %s selected", menuItems[i].c_str());
            }
            ImGui::Spacing();
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Display selected menu info
        ImGui::TextColored(ImVec4(0.933f, 1.0f, 0.8f, 1.0f), "Selected: ");
        ImGui::SameLine();
        if (static_cast<unsigned int>(mSelectedMenuItem) < static_cast<unsigned int>(MenuItem::COUNT)) {
            ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f), "%s",
                menuItems.at(static_cast<unsigned int>(mSelectedMenuItem)).c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Action buttons
        if (ImGui::Button("Confirm Selection", ImVec2(180, 40))) {
            SDL_Log("Confirmed selection: %u", static_cast<unsigned int>(mSelectedMenuItem));
            // Close the menu window to trigger state transition in update()
            mShowMainMenu = false;
        }

        ImGui::SameLine();

#if defined(MAZE_DEBUG)

        if (ImGui::Button("Toggle Demo", ImVec2(180, 40))) {
            showDemoWindow = !showDemoWindow;
        }
#endif

    }
    ImGui::End();

    ImGui::PopStyleColor(10);

    ImGui::PopFont();

    // Draw the game background first so ImGui renders on top of it
    auto& window = *getContext().window;
    window.draw(mBackgroundSprite);
}

bool MenuState::update(float dt, unsigned int subSteps) noexcept
{
    // If menu is visible, just keep it showing (no transitions yet)
    if (mShowMainMenu) {

        return true;
    }

    // Menu was closed by user - process the selected action
    switch (mSelectedMenuItem) {
        case MenuItem::CONTINUE:
            // Check if Pause state is underneath
            if (auto isPauseState = getStack().peekState<PauseState*>(); isPauseState)
            {
                // Pop menu state, returning to paused game
                requestStackPop();
            }
            else
            {
                requestStackPop();
                requestStackPush(States::ID::GAME);
            }
            break;

        case MenuItem::NEW_GAME:
            requestStackPop();
            requestStackPush(States::ID::GAME);
            break;

        case MenuItem::SETTINGS:
            requestStackPush(States::ID::SETTINGS);
            break;

        case MenuItem::SPLASH:
            mShowMainMenu = false;
            requestStackPush(States::ID::SPLASH);
            return true;

        case MenuItem::QUIT:
            requestStateClear();
            break;

        default:
            break;
    }

    mShowMainMenu = true;
    return true;
}

bool MenuState::handleEvent(const SDL_Event& event) noexcept
{

    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.scancode == SDL_SCANCODE_ESCAPE)
        {
            // Commented because returning to Menu from Settings with ESCAPE causes fallthrough effect
            // mShowMainMenu = false;
        }
    }

    return true;
}
