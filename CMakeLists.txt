cmake_minimum_required(VERSION 3.2)

cmake_policy(SET CMP0058 NEW)

project(maze_builder)

set(EXE_NAME "maze_builder")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DEAR_IMGUI_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/imgui.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/imgui_demo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/imgui_draw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/imgui_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/imgui_widgets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_sdl3.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_opengl3.cpp)

set(DEAR_IMGUI_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/imgui.h
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/imconfig.h
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/imgui_internal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/imstb_rectpack.h
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/imstb_textedit.h
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/imstb_truetype.h
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_sdl3.h
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_opengl3.h
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_opengl3_loader.h)

set(DEP_C_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/src/glad_3_0.c
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/noise/noise.c
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/sqlite/sqlite3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/tinycthread/tinycthread.c)

file(GLOB CRAFT_CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp)

option(BUILD_FOR_ANDROID "Building for Android using SDL3" OFF)

string(COMPARE EQUAL "${CMAKE_BUILD_TYPE}" "RelWithDebInfo" ENABLE_DEBUGGING)
string(COMPARE EQUAL "${CMAKE_BUILD_TYPE}" "Debug" ENABLE_DEBUGGING2)

if(ENABLE_DEBUGGING OR ENABLE_DEBUGGING2)
    message(INFO ": Debugging enabled for ${PROJECT_NAME}")
    add_compile_definitions("DEBUGGING")
endif()

# Quick and dirty Android build using SDL3 android-build script
if(BUILD_FOR_ANDROID)
    message(STATUS "Building SDL3 app for Android...")

    if(NOT DEFINED ENV{ANDROID_HOME})
        message(FATAL_ERROR "Set ANDROID_HOME to path of Android Sdk.")
    endif()

    if(NOT DEFINED ENV{ANDROID_NDK_HOME})
        message(FATAL_ERROR "Set ANDROID_NDK_HOME to path of Android Ndk.")
    endif()

    set(ANDROID_JNI_FILES
        ${DEP_C_FILES}
        ${DEAR_IMGUI_SRCS}
        ${DEAR_IMGUI_HEADERS}
        ${CRAFT_SOURCE_FILES})
    set(JAVA_PKG_NAME "com.example.craft")
    set(SDL_DIR_NAME SDL-main)
    set(ANDROID_BUILD_SCRIPT "${CMAKE_BINARY_DIR}/${SDL_DIR_NAME}/build-scripts/androidbuild.sh")

    file(DOWNLOAD https://github.com/libsdl-org/SDL/archive/main.zip SHOW_PROGRESS ${CMAKE_BINARY_DIR}/${SDL_DIR_NAME}.zip STATUS DOWNLOAD_STATUS)
    file(ARCHIVE_EXTRACT INPUT ${SDL_DIR_NAME}.zip)
    # Separate the returned status code, and error message.
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)
    # Check if download was successful.
    if(${STATUS_CODE} EQUAL 0)
        message(STATUS "Download completed successfully!")
    else()
        # Exit CMake if the download failed, printing the error message.
        message(FATAL_ERROR "Error occurred during download: ${ERROR_MESSAGE}")
    endif()

    if(NOT EXISTS ${ANDROID_BUILD_SCRIPT})
        message(FATAL_ERROR ": ${ANDROID_BUILD_SCRIPT} is not valid.")
    endif()

    set(ANDROID_APP_PREFIX ${CMAKE_BINARY_DIR}/${SDL_DIR_NAME}/build/${JAVA_PKG_NAME})
    execute_process(COMMAND bash ${ANDROID_BUILD_SCRIPT} ${JAVA_PKG_NAME} ${ANDROID_JNI_FILES} WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    if(EXISTS ${ANDROID_APP_PREFIX}/app)
        execute_process(COMMAND sed -i -e "s|-lGLESv2|-lGLESv3|" ${ANDROID_APP_PREFIX}/app/jni/src/Android.mk WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        execute_process(COMMAND sed -i -e "s|Game|${PROJECT_NAME}|g" ${ANDROID_APP_PREFIX}/app/src/main/res/values/strings.xml WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        execute_process(COMMAND sed -i -e "s|\"SDLActivity\"|\"CraftActivity\"|g" ${ANDROID_APP_PREFIX}/app/src/main/AndroidManifest.xml WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()
else()
    message(STATUS "Building SDL3 app for Desktop...")

    add_executable(${EXE_NAME} ${DEAR_IMGUI_SRCS} ${DEP_C_FILES} ${CRAFT_CPP_FILES})

    include_directories(deps/glad/include)
    include_directories(deps)
    include_directories(deps/dearimgui)

    find_package(OpenGL REQUIRED)

    # Find/Configure SDL3
    find_package(SDL3 QUIET)
    if(SDL3_FOUND)
        message(STATUS "Using SDL3 via find_package")
    endif()

    # If all else fails, download SDL3
    if(NOT SDL3_FOUND)
        include(FetchContent)
        set(SDL_SHARED TRUE CACHE BOOL "Build a SDL shared library (if available)")
        set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")
        FetchContent_Declare(
            SDL
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG main
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
        )
        message(STATUS "Using SDL3 via FetchContent")
        FetchContent_MakeAvailable(SDL)
        set_property(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif()

    if(MINGW)
        set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
            "C:/Program Files/CURL/lib" "C:/Program Files (x86)/CURL/lib")
        set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH}
            "C:/Program Files/CURL/include" "C:/Program Files (x86)/CURL/include")
    endif()

    find_package(CURL QUIET)
    if(NOT CURL_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            CURL
            GIT_REPOSITORY https://github.com/curl/curl.git
            GIT_TAG master
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE)
        message(STATUS "Using CURL via FetchContent")
        FetchContent_MakeAvailable(CURL)
    endif()

    include_directories(${CURL_SOURCE_DIR}/include)

    if(APPLE)
        target_link_libraries(${EXE_NAME} OpenGL::GL SDL3::SDL3 CURL::libcurl)
    endif()

    if(UNIX)
        target_link_libraries(${EXE_NAME} dl OpenGL::GL SDL3::SDL3 CURL::libcurl)
    endif()

    if(MINGW)
        target_link_libraries(${EXE_NAME} ws2_32.lib OpenGL::GL SDL3::SDL3 CURL::libcurl)
    endif()

    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_BINARY_DIR}")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/textures" DESTINATION "${CMAKE_BINARY_DIR}")
endif()
