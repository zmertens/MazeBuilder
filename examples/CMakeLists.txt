
if(MAZE_BUILDER_EXAMPLES)

    set(DEAR_IMGUI_SOURCES
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui.cpp
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui_demo.cpp
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui_tables.cpp
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui_widgets.cpp
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_sdl3.cpp
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_opengl3.cpp)

    set(DEAR_IMGUI_HEADERS
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui.h
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/imconfig.h
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui_internal.h
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/imstb_rectpack.h
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/imstb_textedit.h
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/imstb_truetype.h
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_sdl3.h
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_opengl3.h
        ${CMAKE_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_opengl3_loader.h)

    if(EMSCRIPTEN)
        message(INFO ": Building examples for the Web!")

        # Check if CMAKE_TOOLCHAIN_FILE is set, throw a fatal error if not
        if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
            message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE is not set. Please specify the path to the Emscripten toolchain file.")
        endif()
    
        # Set global compile flags that will be inherited by all targets
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -matomics -mbulk-memory -pthread")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -matomics -mbulk-memory -pthread")
        
        # Set threading flags for Emscripten
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pthread")
        
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

        # Create a fake Threads target for Emscripten since it doesn't use standard threads
        if(NOT TARGET Threads::Threads)
            add_library(Threads::Threads INTERFACE IMPORTED)
            set_property(TARGET Threads::Threads PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
            set_property(TARGET Threads::Threads PROPERTY INTERFACE_LINK_LIBRARIES "-pthread")
        endif()
        
        # Set variables that FindThreads would normally set
        set(Threads_FOUND TRUE)
        set(CMAKE_THREAD_LIBS_INIT "-pthread")
        set(CMAKE_USE_PTHREADS_INIT TRUE)
        
        list(APPEND BASE_COMPILE_FLAGS "$<$<CXX_COMPILER_ID:Emscripten>:-matomics;-mbulk-memory;-pthread>")

        include(FetchGitRepo)

        # Get box2d
        FetchGitRepo(box2d https://github.com/erincatto/box2d.git v3.1.1)
        # Add Box2D include directory if not already included by find_package
        if(TARGET box2d)
            set_property(TARGET box2d PROPERTY COMPILE_WARNING_AS_ERROR OFF)
            get_target_property(BOX2D_INCLUDE_DIRS box2d INTERFACE_INCLUDE_DIRECTORIES)
        endif()

        # Get SDL
        set(SDL_SHARED FALSE CACHE BOOL "Build a SDL shared library (if available)")
        set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")
        FetchGitRepo(SDL https://github.com/libsdl-org/SDL.git release-3.2.10)
        set_property(DIRECTORY "${CMAKE_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)

        # Get SFML with Emscripten-specific settings
        set(SFML_REPO_URL "https://github.com/zmertens/SFML.git")
        message(STATUS "Fetching SFML from ${SFML_REPO_URL}")
        set(SFML_BUILD_AUDIO ON)
        set(SFML_BUILD_NETWORK OFF)
        set(SFML_BUILD_GRAPHICS OFF)
        set(SFML_BUILD_WINDOW OFF)
        set(SFML_USE_SYSTEM_DEPS OFF)
        # Disable SFML's internal thread finding since we provide our own
        set(SFML_DISABLE_FIND_THREADS ON)
        # Disable problematic audio dependencies for Web builds
        set(SFML_USE_DRM OFF CACHE BOOL "Use DRM for audio")
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
        FetchGitRepo(SFML ${SFML_REPO_URL} almostv3)

    else(EMSCRIPTEN)
        message(INFO ": Building examples for the Desktop!")
        
        # Find threads normally for desktop builds
        find_package(Threads REQUIRED)
        
        include(FetchGitRepo)

        # Add Box2D dependency
        find_package(box2d QUIET)
        if(NOT box2d_FOUND)
            message(STATUS "Getting Box2D via FetchContent")
            FetchGitRepo(box2d https://github.com/erincatto/box2d.git v3.1.1)
        endif()

        # Add Box2D include directory if not already included by find_package
        if(TARGET box2d)
            set_property(TARGET box2d PROPERTY COMPILE_WARNING_AS_ERROR OFF)
            get_target_property(BOX2D_INCLUDE_DIRS box2d INTERFACE_INCLUDE_DIRECTORIES)
        endif()

        # Get SDL
        find_package(SDL3 QUIET)
        if(NOT SDL3_FOUND)
            message(STATUS "Getting SDL via FetchContent")
            set(SDL_SHARED TRUE CACHE BOOL "Build a SDL shared library (if available)")
            set(SDL_STATIC FALSE CACHE BOOL "Build a SDL static library (if available)")
            FetchGitRepo(SDL https://github.com/libsdl-org/SDL.git release-3.2.10)
            set_property(DIRECTORY "${CMAKE_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
        endif()

        # Get SFML from web and build dependencies locally
        set(SFML_BUILD_AUDIO ON)
        set(SFML_BUILD_NETWORK ON)
        set(SFML_BUILD_GRAPHICS OFF)
        set(SFML_BUILD_WINDOW OFF)
        set(SFML_USE_SYSTEM_DEPS OFF)
        set(BUILD_SHARED_LIBS OFF)
        FetchGitRepo(SFML https://github.com/SFML/SFML.git 3.0.1)
        set_property(DIRECTORY "${CMAKE_BINARY_DIR}/_deps/sfml-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif(EMSCRIPTEN)

    add_subdirectory(Voxels)
    add_subdirectory(CLI)
    add_subdirectory(Http)
	add_subdirectory(Physics)
endif()
