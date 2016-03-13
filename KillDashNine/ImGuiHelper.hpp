#ifndef IMGUIHELPER_HPP
#define IMGUIHELPER_HPP

#include <string>
#include <array>
#include <tuple>

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
class KillDashNine;

class ImGuiHelper final
{
public:
    explicit ImGuiHelper(const SdlManager& sdl, ResourceManager& rm);
    virtual ~ImGuiHelper();

    void update(KillDashNine& app);
    void render();
    void cleanUp();

    void updateFrames(const std::string& fps, const std::string secPerFrame);
    void updateDescription(const std::string& desc);

    GuiStates::Shown getState() const;
    void setState(const GuiStates::Shown& state);

    static void ImGui_ImplSdlGL3_RenderDrawLists(ImDrawData* draw_data);
    static const char* ImGui_ImplSdlGL3_GetClipboardText();
    static void ImGui_ImplSdlGL3_SetClipboardText(const char* text);

    void ImGui_ImplSdlGL3_CreateFontsTexture();

    IMGUI_API bool ImGui_ImplSdlGL3_Init();
    IMGUI_API void ImGui_ImplSdlGL3_Shutdown();
    IMGUI_API void ImGui_ImplSdlGL3_NewFrame(); // @todo : priv
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

    bool getShowCrosshairOverlay() const;
    void setShowCrosshairOverlay(bool showCrosshairOverlay);

private:
    typedef struct {
        std::string fps;
        std::string secPerFrame;
    } Frames;

    typedef struct {
        std::string storyline;
        std::string controls;
        std::string play;
        std::string exit;
        bool playSelected;
    } Title;

    enum class Selection {
        MUSIC,
        SOUND,
        CROSSHAIR,
        FULLSCREEN,
        DIFFICULTY,
#if defined(APP_DEBUG)
        Y_AXIS_MV, // might not be needed
        COLLISIONS,
        INVINCIBLE,
        SPEED,
        INF_AMMO,
        STRENGTH,
#endif // defined
        RESTART,
        EXIT,
        TOTAL_OPTIONS
    };

    typedef struct Options {

        Options(const int i) : indexer(i) {}

        int indexer;
        // tuple layout is: text string, selected, on / off
        std::array<std::tuple<std::string, bool, bool>, static_cast<int>(Selection::TOTAL_OPTIONS)> tuples;

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
    bool mShowCrosshairOverlay;

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
    bool isOnExitString() const;
    void handleGuiOptions(KillDashNine& app);
};

#endif // IMGUIHELPER_HPP
