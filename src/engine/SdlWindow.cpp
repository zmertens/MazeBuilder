#include "SdlWindow.hpp"

#include <sstream>
#include <cassert>

#if defined(GAME_DEBUG_MODE)
#include "graphics/GlUtils.hpp"
#endif // defined

/**
 * @brief SdlWindow::SdlWindow
 * @param title
 * @param width = 800u
 * @param height = 600u
 */
SdlWindow::SdlWindow(const char* title, const unsigned int width, const unsigned int height)
: mTitle(title)
, mOpenGlContext(SDL_GL_CONTEXT_PROFILE_CORE)
, mLogPriority(SDL_LOG_PRIORITY_VERBOSE)
{
    SDL_LogSetAllPriority(mLogPriority);

    initWindow(sInitFlags, width, height);

    if (sInitFlags & SDL_INIT_JOYSTICK)
        initJoysticks();
    if (sInitFlags & SDL_INIT_HAPTIC)
        initHaptic();

    // Only load OpenGL functions after a Gl Context is created
    loadGl();

    if (sSamples > 1)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, sSamples);
        glEnable(GL_MULTISAMPLE);
    }

// Requires OpenGL 4.3
// #if defined(GAME_DEBUG_MODE)
//        glEnable(GL_DEBUG_OUTPUT);
//        glDebugMessageCallback(GlUtils::GlDebugCallback, nullptr);
//        glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
//        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION,
//            GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_MEDIUM,
//            GL_DEBUG_SEVERITY_MEDIUM, -1, "Start debugging");
// #endif // defined
}

SdlWindow::~SdlWindow()
{
    if (mSdlWindow != nullptr)
        cleanUp();
}

SdlWindow::SdlWindow(const SdlWindow& other)
{

}

SdlWindow& SdlWindow::operator=(const SdlWindow& other)
{

}

/**
 * @brief SdlWindow::cleanUp
 */
void SdlWindow::cleanUp()
{
    destroyWindow();
    SDL_Quit();
}

/**
 * @brief SdlWindow::initWindow
 * @param flags
 */
void SdlWindow::initWindow(Uint32 flags, unsigned int width, unsigned int height)
{
    if (SDL_Init(flags) < 0)
    { 
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", SDL_GetError());
        throw new std::runtime_error("Could not intialize Sdl");
    }

    // Set up the GL attributes
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, sRedBufferSize);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, sGreenBufferSize);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, sBlueBufferSize);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, sAlphaBufferSize);

    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, sBufferSize);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, sDepthBufferSize);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, sStencilBufferSize);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, mOpenGlContext);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, sOpenGlMajor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, sOpenGlMinor);

#if defined(APP_DESKTOP)
    if (sFullscreen)
    {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(0, 0, &mode);
        mSdlWindow = SDL_CreateWindow(nullptr, 0, 0, mode.w, mode.h, sWinFlags);
    }
    else
    {
        mSdlWindow = SDL_CreateWindow(mTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, sWinFlags);
    }
#elif defined(APP_ANDROID)
    SDL_DisplayMode mode;
    SDL_GetDisplayMode(0, 0, &mode);
    mSdlWindow = SDL_CreateWindow(nullptr, 0, 0, mode.w, mode.h, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
#endif // defined

    if (mSdlWindow == 0)
    {    
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", SDL_GetError());
        throw new std::runtime_error("Sdl Window did not create!\n");
    }

    mGlContext = SDL_GL_CreateContext(mSdlWindow);

    // Sometimes Vsync is enabled by default
    if (sVSync && SDL_GL_SetSwapInterval(1) < 0)
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Vsync mode is not available\n");
    else
        SDL_GL_SetSwapInterval(0);
}

void SdlWindow::initJoysticks()
{
    // setup joystick
    mSdlJoystick = SDL_JoystickOpen(0);
    if (!mSdlJoystick)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Joystick 0 did not initialize\n");
    }
}

void SdlWindow::initHaptic()
{
    mSdlHaptic = SDL_HapticOpenFromJoystick(mSdlJoystick);
    if (!mSdlHaptic)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "This controller does not support haptic feedback\n");
    }
    else
    {
        if (SDL_HapticRumbleInit(mSdlHaptic) < 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL haptic did not initialize the rumble\n");
        }
    }
}

/**
 * @brief SdlWindow::destroyWindow
 */
void SdlWindow::destroyWindow()
{
    SDL_GL_DeleteContext(mGlContext);
    SDL_DestroyWindow(mSdlWindow);
}

/**
 * Only loads on desktop since mobile uses OpenGL ES
 * @brief SdlWindow::loadGl
 */
void SdlWindow::loadGl()
{
#if defined(APP_DESKTOP)
    // Load the OpenGL functions (glLoadGen).
    if (ogl_LoadFunctions() == ogl_LOAD_FAILED)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error loading glLoadGen");
        SDL_Quit();
        throw new std::runtime_error("Could not load OpenGL functions\n");
    }
#endif // defined
}

/**
 * @brief SdlWindow::getSdlInfoString
 * @return
 */
std::string SdlWindow::getSdlInfoString() const
{
    int context, major, minor;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &context);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);

    int multisamples;
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &multisamples);

    assert(major == sOpenGlMajor);
    assert(minor == sOpenGlMinor);
    //assert(multisamples == mSamples);

    std::stringstream ss;
    ss << "\nPrinting SdlWindow info:\n";
    ss << "\nWindow Title: " << mTitle << "\n";
    ss << "Vsync: " << sVSync << "\n";
    ss << "Fullscreen: " << sFullscreen << "\n";
    ss << "Window (width, height): " << "(" << getWindowWidth() << ", " << getWindowHeight() << ")\n";
    ss << "The number of connected joysticks: " << SDL_NumJoysticks() << "\n";
    ss << "The GL context is : " << getContextString(context) << "\n";
    ss << "Major, Minor versions: " << major << ", " << minor << "\n";
    return ss.str();
}

/**
 * @brief SdlWindow::getGlInfoString
 * @return
 */
std::string SdlWindow::getGlInfoString() const
{
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    std::stringstream ss;
    ss << "\n-------------------------------------------------------------\n";
    ss << "GL Vendor:\t\t\t" << vendor;
    ss << "\nGL GLRenderer:\t\t\t" << renderer;
    ss << "\nGL Version:\t\t\t" << version;
    ss << "\nGL Major, Minor:\t\t\t" << major << "." << minor;
    ss << "\nGLSL Version:\t\t\t" << glslVersion;
    ss << "\n-------------------------------------------------------------\n";
    return ss.str();
}

/**
 * @brief SdlWindow::swapBuffers
 */
void SdlWindow::swapBuffers() const
{
    SDL_GL_SwapWindow(mSdlWindow);
}

/**
 * @brief SdlWindow::hapticRumblePlay
 * @param strength
 * @param length
 * @return
 */
bool SdlWindow::hapticRumblePlay(float strength, float length) const
{
    return static_cast<bool>(SDL_HapticRumblePlay(mSdlHaptic, strength, length));
}

/**
 * @brief SdlWindow::buildBufferFromFile
 * @param filename
 * @param bufferSize
 * @return the buffer
 */
unsigned char* SdlWindow::buildBufferFromFile(const std::string& filename, long& bufferSize) const
{
    SDL_RWops* rwOps = SDL_RWFromFile(filename.c_str(), "rb");

    if (rwOps)
    {
        long fileSize = SDL_RWsize(rwOps);
        unsigned char* result = static_cast<unsigned char*>(malloc(fileSize + 1));
        long numberReadTotal = 0;
        long numberRead = 1;

        unsigned char* buffer = result;

        while (numberRead < fileSize && numberRead != 0)
        {
            numberRead = SDL_RWread(rwOps, buffer, 1, (fileSize - numberReadTotal));
            numberReadTotal += numberRead;
            buffer += numberRead;
        }

        SDL_RWclose(rwOps);

        if (numberReadTotal != fileSize)
        {
            free(result);
        }

        result[numberReadTotal] = '\0';

        bufferSize = fileSize;

        return result;
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", SDL_GetError());
        return nullptr;
    }
}

/**
 * @brief SdlWindow::buildStringFromFile
 * @param filename
 * @return
 */
std::string SdlWindow::buildStringFromFile(const std::string& filename) const
{
    std::string returnStr;
    SDL_RWops* rwOps = SDL_RWFromFile(filename.c_str(), "r");

    if (rwOps)
    {
        long fileSize = SDL_RWsize(rwOps);
        char* result = static_cast<char*>(malloc(fileSize + 1));
        long numberReadTotal = 0;
        long numberRead = 1;

        char* buffer = result;

        while (numberRead < fileSize && numberRead != 0)
        {
            numberRead = SDL_RWread(rwOps, buffer, 1, (fileSize - numberReadTotal));
            numberReadTotal += numberRead;
            buffer += numberRead;
        }

        SDL_RWclose(rwOps);

        if (numberReadTotal != fileSize)
        {
            free(result);
        }

        result[numberReadTotal] = '\0';

        returnStr.assign(result, fileSize);
        free(result);
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", SDL_GetError());
    }

    return returnStr;
}

/**
 * @brief SdlWindow::isFullScreen
 * @return
 */
bool SdlWindow::isFullScreen() const
{
    return sFullscreen;
}

/**
 * @brief SdlWindow::getWindowHeight
 * @return
 */
int SdlWindow::getWindowHeight() const
{
    int x, y;
    SDL_GetWindowSize(mSdlWindow, &x, &y);
    return y;
}

/**
 * @brief SdlWindow::getWindowWidth
 * @return
 */
int SdlWindow::getWindowWidth() const
{
    int x, y;
    SDL_GetWindowSize(mSdlWindow, &x, &y);
    return x;
}

/**
 * @brief SdlWindow::getAspectRatio
 * @return
 */
float SdlWindow::getAspectRatio() const
{
    int x, y;
    SDL_GetWindowSize(mSdlWindow, &x, &y);
    return static_cast<float>(x) / static_cast<float>(y);
}

/**
 * @brief SdlWindow::getSdlWindow
 * @return
 */
SDL_Window* SdlWindow::getSdlWindow() const
{
    return mSdlWindow;
}

/**
 * @return
 */
Uint32 SdlWindow::getInitFlags() const
{
    return sInitFlags; 
}

/**
 * priv
 * @brief SdlWindow::getContextString
 * @param context
 * @return
 */
std::string SdlWindow::getContextString(int context) const
{
    switch (context)
    {
        case SDL_GL_CONTEXT_PROFILE_CORE: return "Core GL Context";
        case SDL_GL_CONTEXT_PROFILE_ES: return "ES GL Context";
        case SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG: return "Forward GL Context";
        //case SDL_GL_CONTEXT_DEBUG_FLAG: return "Debug GL Context";
        default: return "Unknown GL context";
    }
}
