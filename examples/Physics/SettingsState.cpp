#include "SettingsState.hpp"

#include <SDL3/SDL.h>

#include <dearimgui/imgui.h>

#include "Font.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "StateStack.hpp"

SettingsState::SettingsState(StateStack& stack, Context context)
    : State(stack, context)
      , mShowText{true}
      , mShowSettingsWindow(true)
      , mBackgroundSprite{context.textures->get(Textures::ID::SPLASH_TITLE_IMAGE)}
{
}

void SettingsState::draw() const noexcept
{
    // Draw the game background FIRST, before any ImGui calls
    const auto& window = *getContext().window;
    window.draw(mBackgroundSprite);

    ImGui::PushFont(getContext().fonts->get(Fonts::ID::LIMELIGHT).get());

    // Apply color schema (matching MenuState)
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.016f, 0.047f, 0.024f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.067f, 0.137f, 0.094f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.118f, 0.227f, 0.161f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.188f, 0.365f, 0.259f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.302f, 0.502f, 0.380f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.537f, 0.635f, 0.341f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.302f, 0.502f, 0.380f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.537f, 0.635f, 0.341f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.745f, 0.863f, 0.498f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.933f, 1.0f, 0.8f, 1.0f));

    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);

    bool windowOpen = mShowSettingsWindow;
    if (ImGui::Begin("Settings", &windowOpen, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Text("Settings");
        ImGui::Separator();
        ImGui::Spacing();

        // Audio Settings Section
        ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f), "Audio Settings:");
        ImGui::Spacing();

        static float masterVolume = 100.0f;
        static float musicVolume = 80.0f;
        static float sfxVolume = 90.0f;

        ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 100.0f, "%.0f%%");
        ImGui::SliderFloat("Music Volume", &musicVolume, 0.0f, 100.0f, "%.0f%%");
        ImGui::SliderFloat("SFX Volume", &sfxVolume, 0.0f, 100.0f, "%.0f%%");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Graphics Settings Section
        ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f), "Graphics Settings:");
        ImGui::Spacing();

        static bool vsync = true;
        static bool fullscreen = false;
        static int selectedResolution = 0;
        const char* resolutions[] = {"800x600", "1024x768", "1280x720", "1920x1080"};

        ImGui::Checkbox("VSync", &vsync);
        ImGui::Checkbox("Fullscreen", &fullscreen);
        ImGui::Combo("Resolution", &selectedResolution, resolutions, IM_ARRAYSIZE(resolutions));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Gameplay Settings Section
        ImGui::TextColored(ImVec4(0.745f, 0.863f, 0.498f, 1.0f), "Gameplay Settings:");
        ImGui::Spacing();

        static float difficulty = 50.0f;
        static bool showFPS = true;
        static bool showDebugInfo = false;

        ImGui::SliderFloat("Difficulty", &difficulty, 0.0f, 100.0f, "%.0f%%");
        ImGui::Checkbox("Show FPS", &showFPS);
        ImGui::Checkbox("Show Debug Info", &showDebugInfo);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Action buttons
        if (ImGui::Button("Apply Settings", ImVec2(150, 40)))
        {
            SDL_Log("Settings applied");
            // Add logic to apply settings
            window.setFullscreen(fullscreen);
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset to Default", ImVec2(150, 40)))
        {
            SDL_Log("Settings reset to default");
            masterVolume = 100.0f;
            musicVolume = 80.0f;
            sfxVolume = 90.0f;
            vsync = true;
            fullscreen = false;
            selectedResolution = 0;
            difficulty = 50.0f;
            showFPS = true;
            showDebugInfo = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Back to Menu", ImVec2(150, 40)))
        {
            SDL_Log("Returning to menu");
            mShowSettingsWindow = false;
        }
    }
    ImGui::End();

    // If user closed the window via the X button, update our state
    if (!windowOpen)
    {
        mShowSettingsWindow = false;
    }

    ImGui::PopStyleColor(10);

    ImGui::PopFont();
}

bool SettingsState::update(float dt, unsigned int subSteps) noexcept
{
    if (mShowSettingsWindow)
    {
        return true;
    }

    // User has closed the window, pop back to menu
    requestStackPop();

    // Return false to stop processing states below
    // This prevents MenuState from being updated in the same frame
    // before the pop actually happens
    return false;
}

bool SettingsState::handleEvent(const SDL_Event& event) noexcept
{
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.scancode == SDL_SCANCODE_ESCAPE)
        {
            mShowSettingsWindow = false;
        }
    }

    return true;
}

