#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <string>
#include <memory>

#include "Config.hpp"

#if defined(APP_DESKTOP)
#include <SDL2/SDL.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_mixer.h>
#include "../extlibs/gl_core_4_5.h"
#elif defined(APP_ANDROID)
#include <SDL.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#endif // defined

namespace SdlWindow
{

/**
 * Possible init flag values:
 * SDL_INIT_TIMER
 * SDL_INIT_AUDIO
 * SDL_INIT_VIDEO
 * SDL_INIT_JOYSTICK
 * SDL_INIT_HAPTIC // force feedback
 * SDL_INIT_GAMECONTROLLER
 * SDL_INIT_EVENTS
 * SDL_INIT_EVERYTHING // all of the above
 * SDL_INIT_NOPARACHUTE
 * compatibility; this flag is ignored
 *
 * Possible window bitfield flag values:
 * SDL_WINDOW_OPENGL
 * SDL_WINDOW_SHOWN
 * SDL_WINDOW_RESIZABLE
 * SDL_WINDOW_FULLSCREEN
 * SDL_FULLSCREEN_DESKTOP
 * SDL_WINDOW_HIDDEN
 * SDL_WINDOW_BORDERLESS
 * SDL_WINDOW_MINIMIZED
 * SDL_WINDOW_MAXIMIZED
 * SDL_WINDOW_INPUT_GRABBED
 * SDL_WINDOW_INPUT_FOCUS
 * SDL_WINDOW_MOUSE_FOCUS
 * SDL_WINDOW_FOREIGN
 * SDL_WINDOW_ALLOW_HIGHDPI
 * SDL_WINDOW_MOUSE_CAPTURE
 *
 * @brief The Window struct
 */
struct Settings {
    Uint32 initFlags;
    Uint32 windowFlags;
    bool vSync;

    Settings(Uint32 init_flags = SDL_INIT_EVERYTHING,
        Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN,
        bool vsync = false)
    : initFlags(init_flags)
    , windowFlags(window_flags)
    , vSync(vsync)
    {

    }
};
} // namespace

/**
 * Uber class that manages SDL2 library and OpenGL context.
 * @brief The SdlManager class
 */
class SdlManager final
{
public:
    typedef std::unique_ptr<SdlManager> Ptr;
public:
    explicit SdlManager(const SdlWindow::Settings& window,
        const std::string& title, const unsigned int width = 0u,
        const unsigned int height = 0u);
    virtual ~SdlManager();

    void cleanUp();

    std::string getSdlInfoString() const;
    std::string getGlInfoString() const;
    void swapBuffers() const;
    bool hapticRumblePlay(float strength, float length) const;

    unsigned char* buildBufferFromFile(const std::string& filename, long& bufferSize) const;
    std::string buildStringFromFile(const std::string& filename) const;

    void toggleFullScreen();
    bool isFullScreen() const;

    void setWindowHeight(const unsigned int height);
    unsigned int getWindowHeight() const;
    void setWindowWidth(const unsigned int width);
    unsigned int getWindowWidth() const;

    float getAspectRatio() const;

    SDL_Window* getSdlWindow() const;
    SdlWindow::Settings getWindowSettings() const;
private:
    SdlWindow::Settings mWindowSettings;
    std::string mTitle;
    unsigned int mWinWidth;
    unsigned int mWinHeight;
    SDL_GLprofile mOpenGlContext;
    SDL_LogPriority mLogPriority;
    int mOpenGlMajor;
    int mOpenGlMinor;
    int mRedBufferSize;
    int mGreenBufferSize;
    int mBlueBufferSize;
    int mAlphaBufferSize;
    int mBufferSize;
    int mDepthBufferSize;
    int mStencilBufferSize;
    int mSamples;

    bool mFullscreen;

    SDL_Window* mSdlWindow;
    SDL_GLContext mGlContext;
    SDL_Joystick* mSdlJoystick;
    SDL_Haptic* mSdlHaptic;
private:
    SdlManager(const SdlManager& other);
    SdlManager& operator=(const SdlManager& other);
    void initWindow(Uint32 flags);
    void initJoysticks();
    void initHaptic();
    void destroyWindow();
    void loadGL();
    std::string getContextString(int context) const;
};

#endif // WINDOW_HPP
