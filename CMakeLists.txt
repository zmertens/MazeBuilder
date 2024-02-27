cmake_minimum_required(VERSION 3.2)

cmake_policy(SET CMP0058 NEW)

project(maze_builder)

set(MAZE_BUILDER_EXE "maze_builder")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

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

option(BUILD_MAZE_TESTS "Build ${PROJECT_NAME} tests" OFF)

file(GLOB PROJECT_CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp)

option(BUILD_MAZE_FOR_ANDROID "Building for Android using SDL3" OFF)

string(COMPARE EQUAL "${CMAKE_BUILD_TYPE}" "RelWithDebInfo" ENABLE_DEBUGGING)
string(COMPARE EQUAL "${CMAKE_BUILD_TYPE}" "Debug" ENABLE_DEBUGGING2)

if(ENABLE_DEBUGGING OR ENABLE_DEBUGGING2)
    message(INFO ": -DDEBUGGING enabled for ${PROJECT_NAME}")
    add_compile_definitions("DEBUGGING")
endif()

# Quick and dirty Android build using SDL3 android-build script
if(BUILD_MAZE_FOR_ANDROID)
    message(INFO ": Building SDL app for Android!")

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
        message(INFO ": SDL download completed successfully!")
    else()
        # Exit CMake if the download failed, printing the error message.
        message(FATAL_ERROR ": Error occurred during download: ${ERROR_MESSAGE}")
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
    message(INFO ": Building SDL app for Desktop!")

    add_executable(${MAZE_BUILDER_EXE} ${DEAR_IMGUI_SRCS} ${DEP_C_FILES} ${PROJECT_CPP_FILES})

    target_compile_features(${MAZE_BUILDER_EXE} PUBLIC cxx_std_17)

    if (WIN32)
        target_include_directories(${MAZE_BUILDER_EXE} PRIVATE "C:\\Program Files (x86)\\SDL3\\include")
        include_directories("C:\\Program Files (x86)\\SDL3\\include")
    endif()

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui)

    find_package(OpenGL REQUIRED)

    # Find/Configure SDL3
    find_package(SDL3 QUIET)
    if(SDL3_FOUND)
        message(INFO ": Getting SDL3 via find_package")
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
        message(INFO ": Getting SDL3 via FetchContent")
        FetchContent_MakeAvailable(SDL)
        set_property(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif()

    # Help find CURL for this platform
    if(MINGW OR WIN32)
        set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
            "C:/Program Files/CURL/lib" "C:/Program Files (x86)/CURL/lib")
        set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH}
            "C:/Program Files/CURL/include" "C:/Program Files (x86)/CURL/include")
    endif()

    find_package(CURL QUIET)
    if(CURL_FOUND)
        message(INFO ": Getting CURL via find_package")
    endif()
    if(NOT CURL_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            CURL
            GIT_REPOSITORY https://github.com/curl/curl.git
            GIT_TAG master
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE)
        message(INFO ": Getting CURL via FetchContent")
        FetchContent_MakeAvailable(CURL)
    endif()

    include_directories(${CURL_SOURCE_DIR}/include)

    if(APPLE)
        target_link_libraries(${MAZE_BUILDER_EXE} OpenGL::GL SDL3::SDL3 CURL::libcurl)
    endif()

    if(UNIX)
        target_link_libraries(${MAZE_BUILDER_EXE} dl OpenGL::GL SDL3::SDL3 CURL::libcurl)
    endif()

    if(MINGW OR WIN32)
        target_link_libraries(${MAZE_BUILDER_EXE} ws2_32.lib OpenGL::GL SDL3::SDL3 CURL::libcurl)
    endif()

    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_BINARY_DIR}")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/textures" DESTINATION "${CMAKE_BINARY_DIR}")
endif()

if(BUILD_MAZE_TESTS)
    find_package(Catch2 3 QUIET)
    if(CATCH2_FOUND)
        message(INFO ": Getting Catch2 via find_package")
    endif()
    if(NOT CATCH2_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.4.0
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE)
        message(INFO ": Getting Catch2 via FetchContent")
        FetchContent_MakeAvailable(Catch2)
        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    endif()

    set(MAZE_BUILDER_ALGO_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/args_handler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/output_writer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/maze_builder_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/bst_maze.cpp)
    set(MAZE_BUILDER_ALGO_LIB "MAZE_BUILDER")
    set(MAZE_BUILDER_TEST_EXE "${MAZE_BUILDER_EXE}_tests")
    file(GLOB MAZE_BUILDER_TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)

    add_library(${MAZE_BUILDER_ALGO_LIB} STATIC ${MAZE_BUILDER_ALGO_FILES})

    # These tests can use the Catch2-provided main
    add_executable(${MAZE_BUILDER_TEST_EXE} ${MAZE_BUILDER_TEST_FILES})
    target_link_libraries(${MAZE_BUILDER_TEST_EXE} PRIVATE ${MAZE_BUILDER_ALGO_LIB} Catch2::Catch2WithMain)
    target_include_directories(${MAZE_BUILDER_TEST_EXE} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    include(CTest)
    include(Catch)
    catch_discover_tests(${MAZE_BUILDER_TEST_EXE})
endif()
