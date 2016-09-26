#ifndef IMGUIHELPER_HPP
#define IMGUIHELPER_HPP

#include <string>
#include <array>
#include <tuple>

#include "imgui/imgui.h"

#include "SdlWindow.hpp"

namespace Gui
{
    enum class States {
        Play,
        Pause,
        Restart
    };
}

class ResourceManager;
class GameImpl;

class ImGuiHelper final
{
public:
    explicit ImGuiHelper(const SdlWindow& sdl, ResourceManager& rm);
    virtual ~ImGuiHelper();

    void update(const GameImpl& game);
    void render();
    void cleanUp();

    Gui::States getState() const;
    void setState(const Gui::States& state);

    static void ImGui_ImplSdlGL3_RenderDrawLists(ImDrawData* draw_data);
    static const char* ImGui_ImplSdlGL3_GetClipboardText();
    static void ImGui_ImplSdlGL3_SetClipboardText(const char* text);

    void ImGui_ImplSdlGL3_CreateFontsTexture();

    IMGUI_API bool ImGui_ImplSdlGL3_Init();
    IMGUI_API void ImGui_ImplSdlGL3_Shutdown();
    IMGUI_API void ImGui_ImplSdlGL3_NewFrame();
    IMGUI_API bool ImGui_ImplSdlGL3_ProcessEvent(SDL_Event* event);
    // Use if you want to reset your rendering device without losing ImGui state.
    IMGUI_API void ImGui_ImplSdlGL3_InvalidateDeviceObjects();
    IMGUI_API bool ImGui_ImplSdlGL3_CreateDeviceObjects();

    void setPlayerHealth(const std::string& health);
private:
    static GLint sShaderHandle;
    static GLint sVertHandle;
    static GLint sFragHandle;
    static GLint sAttribLocationTex;
    static GLint sAttribLocationProjMat;
    static GLint sAttribLocationPosition;
    static GLint sAttribLocationUv;
    static GLint sAttribLocationColor;
    static GLuint sVboHandle;
    static GLuint sVaoHandle;
    static GLuint sElementsHandle;

    const SdlWindow& cSdl;
    ResourceManager& mResourceManager;

    ImGuiWindowFlags mOverlayFlags;
    ImColor mSelected;
    float mOverlayAlpha;
    bool mShowHealth;

    Gui::States mState;

    std::string mHealth;

    GLuint g_FontTexture;
    double g_Time;
    bool g_MousePressed[3];
    float g_MouseWheel;
private:
    ImGuiHelper(const ImGuiHelper& other);
    ImGuiHelper& operator=(const ImGuiHelper& other);
};

#endif // IMGUIHELPER_HPP
