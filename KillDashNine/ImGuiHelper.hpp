#ifndef IMGUIHELPER_HPP
#define IMGUIHELPER_HPP

#include <string>

#include "extlibs/imgui/imgui.h"

#include "engine/SdlManager.hpp"
#include "engine/AABB.hpp"

class ResourceManager;
//class SdlManager; // use forward declare once shader / tex have been set up

class ImGuiHelper final
{
public:
    explicit ImGuiHelper(const SdlManager& sdl, ResourceManager& rm);
    virtual ~ImGuiHelper();

    float getZ_PositionSliderValue() const;

    void render();

    void cleanUp();

    bool isInGUI_Window(const glm::vec3& coords) const;

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

    const SdlManager& cSdlManager;
    ResourceManager& mResourceManager;

    bool mShowComboBoxWindow;
    bool mShow_Z_PositionWindow;
    bool mShow_Overlay;
    float mBackgroundAlpha;
    float mComboWidgetHeight;
    float mSliderWidgetHeight;
    float mSliderGrabMinSize;
    float mZ_PositionSliderValue;
    AABB mComboWindow_AABB;
    AABB mSliderWindow_AABB;

    ImGuiWindowFlags mImGuiWindowFlags;

    GLuint g_FontTexture;
    double g_Time;
    bool g_MousePressed[3];
    float g_MouseWheel;

    ImVec2 mDefaultStyle;
    ImVec2 mDefaultPadding;
    ImVec2 mDefaultItemSpacing;
};

#endif // IMGUIHELPER_HPP
