if(MAZE_BUILDER_EXAMPLES)

    # Some third-party deps (for example ogg pulled by SFML audio) still declare
    # very old cmake_minimum_required values. Keep compatibility enabled for them.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum policy version for legacy third-party dependencies" FORCE)

    set(BOX2D_REPO_URL https://github.com/erincatto/box2d.git)
    set(BOX2D_VERSION v3.1.1)
    set(SFML_REPO_URL https://github.com/zmertens/SFML.git)
    set(SFML_VERSION almostv3)

    message(INFO ": Fetching Box2D from ${BOX2D_REPO_URL}")
    message(INFO ": Fetching SFML from ${SFML_REPO_URL}")

    if(EMSCRIPTEN)
        message(INFO " - Building examples for the Web!")

        # Check if CMAKE_TOOLCHAIN_FILE is set, throw a fatal error if not
        if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
            message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE is not set. Please specify the path to the Emscripten toolchain file.")
        endif()

        # Set baseline WebAssembly flags for all Emscripten targets.
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -matomics -mbulk-memory")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -matomics -mbulk-memory")

        # Fix for Emscripten/WebAssembly type detection issues in Ogg/Vorbis
        set(SIZEOF_SHORT 2 CACHE INTERNAL "Size of short")
        set(SIZEOF_INT 4 CACHE INTERNAL "Size of int")
        set(SIZEOF_LONG 4 CACHE INTERNAL "Size of long")
        set(SIZEOF_LONG_LONG 8 CACHE INTERNAL "Size of long long")

        set(HAVE_STDINT_H 1 CACHE INTERNAL "Have stdint.h")
        set(HAVE_STDBOOL_H 1 CACHE INTERNAL "Have stdbool.h")
        set(HAVE_INTTYPES_H 1 CACHE INTERNAL "Have inttypes.h")

        set(HAVE_STDLIB_H 1 CACHE INTERNAL "Have stdlib.h")
        set(HAVE_STRING_H 1 CACHE INTERNAL "Have string.h")
        set(HAVE_MEMORY_H 1 CACHE INTERNAL "Have memory.h")
        set(HAVE_SYS_TYPES_H 1 CACHE INTERNAL "Have sys/types.h")

        # Create a fake Threads target for Emscripten.
        if(NOT TARGET Threads::Threads)
            add_library(Threads::Threads INTERFACE IMPORTED)
        endif()

        # Set variables that FindThreads would normally set
        set(Threads_FOUND TRUE)
        if(EMSCRIPTEN)
            set(CMAKE_THREAD_LIBS_INIT "-pthread")
            set(CMAKE_USE_PTHREADS_INIT TRUE)
        else()
            set(CMAKE_THREAD_LIBS_INIT "")
            set(CMAKE_USE_PTHREADS_INIT FALSE)
        endif()

        include(FetchGitRepo)

        # Get box2d
        # Emscripten does not expose x86 SSE by default; disable Box2D SIMD so
        # compat/emmintrin.h is not pulled in during C compilation.
        set(BOX2D_DISABLE_SIMD ON CACHE BOOL "Disable Box2D SIMD for Emscripten builds" FORCE)
        FetchGitRepo(box2d ${BOX2D_REPO_URL} ${BOX2D_VERSION})
        # Add Box2D include directory if not already included by find_package
        if(TARGET box2d)
            set_property(TARGET box2d PROPERTY COMPILE_WARNING_AS_ERROR OFF)
            get_target_property(BOX2D_INCLUDE_DIRS box2d INTERFACE_INCLUDE_DIRECTORIES)
        endif()

        # Get SFML with Emscripten-specific settings
        # SFML audio brings in legacy ogg/vorbis CMake projects and is not needed
        # for the current web examples.
        set(SFML_BUILD_AUDIO ON)
        set(SFML_BUILD_NETWORK ON)
        set(SFML_BUILD_GRAPHICS OFF)
        set(SFML_BUILD_WINDOW OFF)
        set(SFML_USE_SYSTEM_DEPS OFF)
        # Disable SFML's internal thread finding since we provide our own
        set(SFML_DISABLE_FIND_THREADS ON)
        # Disable problematic audio dependencies for Web builds
        set(SFML_USE_DRM OFF CACHE BOOL "Use DRM for audio")
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
        FetchGitRepo(SFML ${SFML_REPO_URL} ${SFML_VERSION})

    else()
        message(INFO ": Building examples for the Desktop!")

        # Find threads normally for desktop builds
        find_package(Threads REQUIRED)

        include(FetchGitRepo)

        # Add Box2D dependency
        FetchGitRepo(box2d ${BOX2D_REPO_URL} ${BOX2D_VERSION})

        # Add Box2D include directory if not already included by find_package
        if(TARGET box2d)
            set_property(TARGET box2d PROPERTY COMPILE_WARNING_AS_ERROR OFF)
            get_target_property(BOX2D_INCLUDE_DIRS box2d INTERFACE_INCLUDE_DIRECTORIES)
        endif()

        # Get SFML from web and build dependencies locally
        set(SFML_BUILD_AUDIO ON)
        set(SFML_BUILD_NETWORK ON)
        set(SFML_BUILD_GRAPHICS ON)
        set(SFML_BUILD_WINDOW ON)
        set(SFML_USE_SYSTEM_DEPS OFF)
        set(BUILD_SHARED_LIBS OFF)
        FetchGitRepo(SFML ${SFML_REPO_URL} ${SFML_VERSION})

        if(TARGET sfml-graphics AND TARGET SheenBidi)
            add_dependencies(sfml-graphics SheenBidi)
        endif()

        if(TARGET sfml-graphics)
            foreach(_sfml_graphics_dep harfbuzz freetype)
                if(TARGET ${_sfml_graphics_dep})
                    add_dependencies(sfml-graphics ${_sfml_graphics_dep})
                endif()
            endforeach()
        endif()

        if(TARGET sfml-network AND TARGET libssh2_static)
            add_dependencies(sfml-network libssh2_static)
        endif()

        if(TARGET sfml-network)
            foreach(_sfml_network_dep mbedtls mbedcrypto mbedx509)
                if(TARGET ${_sfml_network_dep})
                    add_dependencies(sfml-network ${_sfml_network_dep})
                endif()
            endforeach()
        endif()
    endif(EMSCRIPTEN)

    add_subdirectory(CLI)
    add_subdirectory(Http)
    add_subdirectory(Amazing)
endif()
