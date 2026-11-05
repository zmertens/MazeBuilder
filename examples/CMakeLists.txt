if(MAZE_BUILDER_EXAMPLES)
    # Some third-party deps (for example ogg pulled by SFML audio) still declare
    # very old cmake_minimum_required values. Keep compatibility enabled for them.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum policy version for legacy third-party dependencies" FORCE)

    set(BOX2D_REPO_URL https://github.com/erincatto/box2d.git)
    set(BOX2D_VERSION v3.1.1)
    set(SFML_REPO_URL https://github.com/zmertens/SFML.git)
    set(SFML_VERSION almostv3)

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
    else()
        message(INFO ": Building examples for the Desktop!")

        # Find threads normally for desktop builds
        find_package(Threads REQUIRED)

        include(FetchGitRepo)

        # Add Box2D dependency
        message(INFO ": Fetching Box2D from ${BOX2D_REPO_URL}")
        FetchGitRepo(box2d ${BOX2D_REPO_URL} ${BOX2D_VERSION})

        # Add Box2D include directory if not already included by find_package
        if(TARGET box2d)
            set_property(TARGET box2d PROPERTY COMPILE_WARNING_AS_ERROR OFF)
            get_target_property(BOX2D_INCLUDE_DIRS box2d INTERFACE_INCLUDE_DIRECTORIES)
        endif()

        # Get SFML from web and build dependencies locally
        set(SFML_BUILD_AUDIO OFF)
        set(SFML_BUILD_NETWORK OFF)
        set(SFML_BUILD_GRAPHICS ON)
        set(SFML_BUILD_WINDOW ON)
        set(SFML_USE_SYSTEM_DEPS OFF)
        set(BUILD_SHARED_LIBS OFF)
        FetchGitRepo(SFML ${SFML_REPO_URL} ${SFML_VERSION})

        add_subdirectory(Amazing)
    endif(EMSCRIPTEN)

    add_subdirectory(CLI)
endif()
