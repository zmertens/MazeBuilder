#ifndef SDLWINDOW_HPP
#define SDLWINDOW_HPP

#include <string>
#include <cstring>
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
    explicit SdlWindow(const char* title, 
        const unsigned int width = 800u,
        const unsigned int height = 600u);
    virtual ~SdlWindow();

    void cleanUp();

    std::string getSdlInfoString() const;
    std::string getGlInfoString() const;
    
    void swapBuffers() const;
    
    bool hapticRumblePlay(float strength, float length) const;

    unsigned char* buildBufferFromFile(const std::string& filename, long& bufferSize) const;
    std::string buildStringFromFile(const std::string& filename) const;

    bool isFullScreen() const;
    int getWindowHeight() const;
    int getWindowWidth() const;
    float getAspectRatio() const;
    SDL_Window* getSdlWindow() const;
    Uint32 getInitFlags() const;
private:
    static constexpr Uint32 sInitFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO;
    static constexpr Uint32 sWinFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN*/;
    static constexpr bool sVSync = false;
    static constexpr bool sFullscreen = (sWinFlags & SDL_WINDOW_FULLSCREEN) || (sWinFlags & SDL_WINDOW_FULLSCREEN_DESKTOP);

    static constexpr int sOpenGlMajor = 4;
    static constexpr int sOpenGlMinor = 5;
    static constexpr int sRedBufferSize = 8;
    static constexpr int sGreenBufferSize = 8;
    static constexpr int sBlueBufferSize = 8;
    static constexpr int sAlphaBufferSize = 8;
    static constexpr int sBufferSize = 24;
    static constexpr int sDepthBufferSize = 8;
    static constexpr int sStencilBufferSize = 8;
    static constexpr int sSamples = 4;

    const char* mTitle;

    SDL_GLprofile mOpenGlContext;
    SDL_LogPriority mLogPriority;

    SDL_Window* mSdlWindow;
    SDL_GLContext mGlContext;
    SDL_Joystick* mSdlJoystick;
    SDL_Haptic* mSdlHaptic;
private:
    SdlWindow(const SdlWindow& other);
    SdlWindow& operator=(const SdlWindow& other);
    void initWindow(Uint32 flags, unsigned int width, unsigned int height);
    void initJoysticks();
    void initHaptic();
    void destroyWindow();
    void loadGl();
    std::string getContextString(int context) const;
};

#endif // SDLWINDOW_HPP
