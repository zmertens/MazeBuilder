
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
    
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -matomics -mbulk-memory")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -matomics -mbulk-memory")

        include(FetchGitRepo)

        # Get box2d
        FetchGitRepo(box2d https://github.com/erincatto/box2d.git v3.1.0)
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

        # Get SFML
        set(SFML_REPO_URL "https://github.com/zmertens/SFML.git")
        message(INFO ": Fetching SFML from ${SFML_REPO_URL}")
        set(SFML_BUILD_AUDIO ON)
        set(SFML_BUILD_NETWORK OFF)
        set(SFML_BUILD_GRAPHICS OFF)
        set(SFML_BUILD_WINDOW OFF)
        set(SFML_USE_SYSTEM_DEPS OFF)
        FetchGitRepo(SFML ${SFML_REPO_URL} almostv3)

    else(EMSCRIPTEN)
        message(INFO ": Building examples for the Desktop!")
        
        include(FetchGitRepo)

        # Add Box2D dependency
        find_package(box2d QUIET)
        if(NOT box2d_FOUND)
            message(INFO ": Getting Box2D via FetchContent")
            FetchGitRepo(box2d https://github.com/erincatto/box2d.git v3.1.0)
        endif()

        # Add Box2D include directory if not already included by find_package
        if(TARGET box2d)
            set_property(TARGET box2d PROPERTY COMPILE_WARNING_AS_ERROR OFF)
            get_target_property(BOX2D_INCLUDE_DIRS box2d INTERFACE_INCLUDE_DIRECTORIES)
        endif()

        # Get SDL
        find_package(SDL3 QUIET)
        if(NOT SDL3_FOUND)
            message(INFO ": Getting SDL via FetchContent")
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
    # @TODO Recommit Physics examples when box2d is next released
	#add_subdirectory(Physics)
endif()
