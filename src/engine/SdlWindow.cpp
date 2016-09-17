#include "SdlWindow.hpp"

#include <sstream>
#include <cassert>

#if defined(APP_DEBUG)
#include "graphics/GlUtils.hpp"
#endif // defined

/**
 * @brief SdlWindow::SdlWindow
 * @param window
 * @param title
 * @param width = 600u
 * @param height = 600u
 */
SdlWindow::SdlWindow(Uint32 initFlags, Uint32 winFlags, bool vsync,
    const std::string& title, const unsigned int width,
    const unsigned int height)
: mInitFlags(initFlags)
, mWinFlags(winFlags)
, mVSync(vsync)
, mTitle(title)
, mWinWidth(width)
, mWinHeight(height)
, mOpenGlContext(SDL_GL_CONTEXT_PROFILE_CORE)
, mLogPriority(SDL_LOG_PRIORITY_VERBOSE)
, mOpenGlMajor(4)
, mOpenGlMinor(5)
, mRedBufferSize(8)
, mGreenBufferSize(8)
, mBlueBufferSize(8)
, mAlphaBufferSize(8)
, mBufferSize(24)
, mDepthBufferSize(8)
, mStencilBufferSize(8)
, mSamples(4)
, mFullscreen((mWinFlags & SDL_WINDOW_FULLSCREEN) ||
    (mWinFlags & SDL_WINDOW_FULLSCREEN_DESKTOP))
{
    SDL_LogSetAllPriority(mLogPriority);

    initWindow(mInitFlags);
    if (mInitFlags & SDL_INIT_JOYSTICK)
        initJoysticks();
    if (mInitFlags & SDL_INIT_HAPTIC)
        initHaptic();

    // Only load OpenGL functions after SDL window is created
    loadGL();

    if (mSamples > 1)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, mSamples);
        glEnable(GL_MULTISAMPLE);
    }

#if defined(APP_DEBUG)
       glEnable(GL_DEBUG_OUTPUT);
       glDebugMessageCallback(GlUtils::GlDebugCallback, nullptr);
       glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
       glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION,
           GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_MEDIUM,
           GL_DEBUG_SEVERITY_MEDIUM, -1, "Start debugging");
#endif // defined
}

SdlWindow::~SdlWindow()
{
    if (mSdlWindow)
        cleanUp();
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
void SdlWindow::initWindow(Uint32 flags)
{
    if (SDL_Init(flags) < 0)
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, SDL_GetError());

    // Set up the GL attributes
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, mRedBufferSize);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, mGreenBufferSize);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, mBlueBufferSize);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, mAlphaBufferSize);

    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, mBufferSize);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, mDepthBufferSize);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, mStencilBufferSize);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, mOpenGlContext);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, mOpenGlMajor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, mOpenGlMinor);

#if defined(APP_DESKTOP)
    if (mFullscreen)
    {
        mSdlWindow = SDL_CreateWindow(mTitle.c_str(), 0, 0,
            mWinWidth, mWinHeight,
            mWinFlags);
    }
    else
    {
        mSdlWindow = SDL_CreateWindow(mTitle.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            mWinWidth, mWinHeight,
            mWinFlags);
    }
#elif defined(APP_ANDROID)
    SDL_DisplayMode mode;
    SDL_GetDisplayMode(0, 0, &mode);
    mWinWidth = static_cast<float>(mode.w);
    mWinHeight = static_cast<float>(mode.h);

    mSdlWindow = SDL_CreateWindow(nullptr, 0, 0, mode.w, mode.h, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
#endif // defined

    if (mSdlWindow == 0)
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, SDL_GetError());

    mGlContext = SDL_GL_CreateContext(mSdlWindow);

    // Sometimes Vsync is enabled by default
    if (mVSync && SDL_GL_SetSwapInterval(1) < 0)
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
 * @brief SdlWindow::loadGL
 */
void SdlWindow::loadGL()
{
#if defined(APP_DESKTOP)
    // Load the OpenGL functions (glLoadGen).
    if (ogl_LoadFunctions() == ogl_LOAD_FAILED)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error loading glLoadGen");
        destroyWindow();
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

    assert(major == mOpenGlMajor);
    assert(minor == mOpenGlMinor);
    //assert(multisamples == mSamples);

    std::string contextStr = getContextString(context);

    std::stringstream ss;
    ss << "\nPrinting SdlWindow info:\n";
    ss << "\nWindow Title: " << mTitle << "\n";
    ss << "Vsync: " << mVSync << "\n";
    ss << "Fullscreen: " << mFullscreen << "\n";
    ss << "Window (width, height): " << "(" << mWinWidth << ", " << mWinHeight << ")\n";
    ss << "The number of connected joysticks: " << SDL_NumJoysticks() << "\n";
    ss << "The GL context is : " << contextStr << "\n";
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
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, SDL_GetError());
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
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, SDL_GetError());
    }

    return returnStr;
}

/**
 * @brief SdlWindow::switchFullScreenMode
 */
void SdlWindow::toggleFullScreen()
{
    mFullscreen = !mFullscreen;
    if (mFullscreen)
        SDL_SetWindowFullscreen(mSdlWindow, SDL_TRUE);
    else
        SDL_SetWindowFullscreen(mSdlWindow, SDL_FALSE);
}

/**
 * @brief SdlWindow::isFullScreen
 * @return
 */
bool SdlWindow::isFullScreen() const
{
    return mFullscreen;
}

/**
 * @brief SdlWindow::setWindowHeight
 * @param height
 */
void SdlWindow::setWindowHeight(const unsigned int height)
{
    mWinHeight = height;
}

/**
 * @brief SdlWindow::getWindowHeight
 * @return
 */
unsigned int SdlWindow::getWindowHeight() const
{
    return mWinHeight;
}

/**
 * @brief SdlWindow::setWindowWidth
 * @param width
 */
void SdlWindow::setWindowWidth(const unsigned int width)
{
    mWinWidth = width;
}

/**
 * @brief SdlWindow::getWindowWidth
 * @return
 */
unsigned int SdlWindow::getWindowWidth() const
{
    return mWinWidth;
}

/**
 * @brief SdlWindow::getAspectRatio
 * @return
 */
float SdlWindow::getAspectRatio() const
{
    return static_cast<float>(mWinWidth) / static_cast<float>(mWinHeight);
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
    return mInitFlags; 
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
