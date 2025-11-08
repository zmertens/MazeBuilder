#include "SettingsState.hpp"

#include <SDL3/SDL.h>

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_sdlrenderer3.h>

#include "Font.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "StateStack.hpp"

SettingsState::SettingsState(StateStack& stack, Context context)
    : State(stack, context)
      , mShowText{true}
      , mShowSettingsWindow(true)
      , mBackgroundSprite{context.textures->get(Textures::ID::SDL_BLOCKS)}
{
}

void SettingsState::draw() const noexcept
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::PushFont(getContext().fonts->get(Fonts::ID::NUNITO_SANS).get());

    // Custom settings window with color schema
    if (mShowSettingsWindow) {
        // Apply color schema (matching MenuState)
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
        ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Settings", &mShowSettingsWindow, ImGuiWindowFlags_NoCollapse)) {
            ImGui::Text("Game Settings");
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
            if (ImGui::Button("Apply Settings", ImVec2(150, 40))) {
                SDL_Log("Settings applied");
                // Add logic to apply settings
            }

            ImGui::SameLine();

            if (ImGui::Button("Reset to Default", ImVec2(150, 40))) {
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

            if (ImGui::Button("Back to Menu", ImVec2(150, 40))) {
                SDL_Log("Returning to menu");
                mShowSettingsWindow = false;
            }
        }
        ImGui::End();

        ImGui::PopStyleColor(10);
    }

    ImGui::PopFont();

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), getContext().window->getRenderer());

    auto& window = *getContext().window;

    window.draw(mBackgroundSprite);
}

bool SettingsState::update(float dt) noexcept
{
    if (!mShowSettingsWindow)
    {
        requestStackPop();
    }

    return true;
}

bool SettingsState::handleEvent(const SDL_Event& event) noexcept
{
    ImGui_ImplSDL3_ProcessEvent(&event);

    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.scancode == SDL_SCANCODE_ESCAPE)
        {
            mShowSettingsWindow = false;
        }
    }

    return true;
}

