#ifndef SDLWINDOW_HPP
#define SDLWINDOW_HPP

#include <string>
#include <memory>

#include "Config.hpp"

#if defined(APP_DESKTOP)
#include <SDL2/SDL.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_mixer.h>
#include "gl_core_4_5.h"
#elif defined(APP_ANDROID)
#include <SDL.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#endif // defined

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
 * Uber class that manages SDL2 window and OpenGL context.
 * @brief The SdlWindow class
 */
class SdlWindow final
{
public:
    typedef std::unique_ptr<SdlWindow> Ptr;
public:
    explicit SdlWindow(Uint32 initFlags, Uint32 winFlags, bool vsync,
        const std::string& title, const unsigned int width = 800u,
        const unsigned int height = 600u);
    virtual ~SdlWindow();

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

    Uint32 getInitFlags() const;
private:
    Uint32 mInitFlags;
    Uint32 mWinFlags;
    bool mVSync;
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
    SdlWindow(const SdlWindow& other);
    SdlWindow& operator=(const SdlWindow& other);
    void initWindow(Uint32 flags);
    void initJoysticks();
    void initHaptic();
    void destroyWindow();
    void loadGL();
    std::string getContextString(int context) const;
};

#endif // SDLWINDOW_HPP
