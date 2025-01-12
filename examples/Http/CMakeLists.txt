# Set the name of the executable
set(HTTP_EXE "mazebuilderhttp")
if(EMSCRIPTEN)
    add_executable(${HTTP_EXE} ${DEAR_IMGUI_SOURCES} main.cpp)
    set_target_properties(${HTTP_EXE} PROPERTIES SUFFIX ".html" LINK_FLAGS "-lembind")

    include(FetchGitRepo)

    # Get SDL
    set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")
    FetchGitRepo(SDL https://github.com/libsdl-org/SDL.git preview-3.1.3)

    # Get SFML
    set(SFML_REPO_URL "https://github.com/SFML/SFML.git")
    message(INFO ": Fetching SFML from ${SFML_REPO_URL}")
    set(SFML_BUILD_AUDIO OFF)
    set(SFML_BUILD_NETWORK ON)
    set(SFML_BUILD_GRAPHICS OFF)
    set(SFML_BUILD_WINDOW OFF)
    set(SFML_USE_SYSTEM_DEPS OFF)
    FetchGitRepo(SFML ${SFML_REPO_URL} 3.0.0)

    target_link_libraries(${HTTP_EXE} PRIVATE ${MAZE_BUILDER_CORE_LIB}_static sfml-network SDL3::SDL3)
    target_include_directories(${HTTP_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/emscripten_local)
    target_compile_features(${HTTP_EXE} PRIVATE cxx_std_17)
    target_include_directories(${HTTP_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps)
    target_include_directories(${HTTP_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/dearimgui)
    target_include_directories(${HTTP_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
    
    target_link_options(${HTTP_EXE} PRIVATE
    	"SHELL:-s WASM=1"
        "SHELL:-s FULL_ES3=1 -s MAX_WEBGL_VERSION=2 -s MODULARIZE=1 -s EXPORT_ES6=1 -s ASYNCIFY -O3"
        #"SHELL:--shell-file ${CMAKE_SOURCE_DIR}/deps/emscripten_local/shell_minimal.html"
    )
else()
    message(INFO ": Building ${HTTP_EXE} for the Desktop!")
    add_executable(${HTTP_EXE} main.cpp)
    target_link_libraries(${HTTP_EXE} PRIVATE ${MAZE_BUILDER_CORE_LIB}_static)
    target_include_directories(${HTTP_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
    target_include_directories(${HTTP_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps)
    target_compile_features(${HTTP_EXE} PRIVATE cxx_std_17)

    # Get SFML from the web
    find_package(SFML 3.0.0 COMPONENTS network)
    if(NOT SFML_FOUND)
        include(FetchGitRepo)
        set(SFML_REPO_URL "https://github.com/SFML/SFML.git")
        message(INFO ": Fetching SFML from ${SFML_REPO_URL}")
        set(SFML_BUILD_AUDIO ON)
        set(SFML_BUILD_NETWORK ON)
        set(SFML_BUILD_GRAPHICS OFF)
        set(SFML_BUILD_WINDOW OFF)
        set(SFML_USE_SYSTEM_DEPS OFF)
        FetchGitRepo(SFML ${SFML_REPO_URL} 3.0.0)
    endif()

    target_link_libraries(${HTTP_EXE} PRIVATE sfml-network)
endif()
