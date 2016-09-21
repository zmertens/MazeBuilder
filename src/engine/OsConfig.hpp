// identify the operating system:
// http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system

#ifndef OSCONFIG_HPP
#define OSCONFIG_HPP

#if defined(_WIN32)

    #define APP_MS_WINDOWS

#elif defined(__APPLE__) && defined(__MACH__)

    #define APP_APPLE

#elif defined(__unix__)

    #if defined(__ANDROID__)

        #define APP_ANDROID

    #elif defined(__linux__)

        #define APP_LINUX

    #else

        #error This UNIX operating system is not supported

    #endif // defined

#else

    #error This operating system is not supported

#endif // defined

#if defined(APP_MS_WINDOWS)

    #define APP_DESKTOP

#elif defined(APP_APPLE)

    #define APP_DESKTOP

#elif defined(APP_LINUX)

    #define APP_DESKTOP

#endif // defined

#endif // OSCONFIG_HPP

