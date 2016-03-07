#ifndef IMGUIHELPER_HPP
#define IMGUIHELPER_HPP

#include <string>

#include "extlibs/imgui/imgui.h"

#include "engine/SdlManager.hpp"

namespace GuiStates
{
    enum class Shown {
        TITLE,
        OPTIONS,
        PLAY
    };
}

namespace
{

}

class ResourceManager;

class ImGuiHelper final
{
public:
    explicit ImGuiHelper(const SdlManager& sdl, ResourceManager& rm);
    virtual ~ImGuiHelper();

    void render();
    void cleanUp();

    void updateFrames(const std::string& fps, const std::string secPerFrame);
    void updateDescription(const std::string& desc);

    GuiStates::Shown getState() const;
    void setState(const GuiStates::Shown& state);
    void naturalStateUpdate();

    void reactToUpArrow();
    void reactToDownArrow();

    bool isOnExitString() const;

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

    bool getShowFramesOverlay() const;
    void setShowFramesOverlay(bool showFramesOverlay);

    bool getShowPlayerStatsOverlay() const;
    void setShowPlayerStatsOverlay(bool showPlayerStatsOverlay);

    bool getShowTitleOverlay() const;
    void setShowTitleOverlay(bool showTitleOverlay);

    bool getShowOptionsOverlay() const;
    void setShowOptionsOverlay(bool showOptionsOverlay);

    bool getShowDescOverlay() const;
    void setShowDescOverlay(bool showDescOverlay);

private:
    typedef struct {
        std::string fps;
        std::string secPerFrame;
    } Frames;

    typedef struct {
        std::string title;
        std::string storyline;
        std::string controls;
        std::string play;
        std::string exit;
        bool playSelected;
    } Title;

    typedef struct {
        // will need to get crafty with this,
        // possibly use a tuple<string, string, bool>
        // to have 2 versions of strings depending
        // wether or not option is off or on
        std::string music;
        std::string sounds;
        std::string crosshair; // rendered using ImGui
        std::string difficulty;
        // debug
        std::string yAxisMovement;
        std::string collisions;
        std::string invincible;
        std::string speed;
        std::string infAmmo;
        std::string str;
        // debug
        std::string restart;
        std::string exit;
        // use offsetof to index the struct string?
    } Options;

    typedef struct {
        std::string desc;
    } Description;

    typedef struct {
        std::string health;
        std::string ammo;
        std::string state;
    } PlayerStats;

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

    ImGuiWindowFlags mOverlayFlags;
    ImColor mSelected;

    bool mShowFramesOverlay;
    bool mShowPlayerStatsOverlay;
    bool mShowTitleOverlay;
    bool mShowOptionsOverlay;
    bool mShowDescOverlay;

    float mOverlayAlpha;

    GuiStates::Shown mState;

    Frames mFrames;
    Title mTitle;
    Options mOptions;
    Description mDesc;
    PlayerStats mStats;

    GLuint g_FontTexture;
    double g_Time;
    bool g_MousePressed[3];
    float g_MouseWheel;
private:
    ImGuiHelper(const ImGuiHelper& other);
    ImGuiHelper& operator=(const ImGuiHelper& other);
};

#endif // IMGUIHELPER_HPP
