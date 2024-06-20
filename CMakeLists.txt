cmake_minimum_required(VERSION 3.2...3.5)

cmake_policy(SET CMP0058 NEW)
cmake_policy(SET CMP0072 NEW)

project(maze_builder)

set(MAZE_BUILDER_EXE "maze_builder")
set(MAZE_BUILDER_TEST_EXE "maze_builder_tests")

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

set(DEPENDENCY_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/src/glad_3_0.c
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/noise/noise.c
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/sqlite/sqlite3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/tinycthread/tinycthread.c)

option(BUILD_MAZE_TESTS "Build ${PROJECT_NAME} tests" OFF)

set(MAZE_ALGO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/args_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/sidewinder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/binary_tree.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/sidewinder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/grid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/cell.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/writer.cpp)

set(CRAFT_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/craft.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/cube.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/db.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/item.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/matrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/ring.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/sign.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/world.cpp)

string(COMPARE EQUAL "${CMAKE_BUILD_TYPE}" "RelWithDebInfo" ENABLE_DEBUGGING)
string(COMPARE EQUAL "${CMAKE_BUILD_TYPE}" "Debug" ENABLE_DEBUGGING2)

if(ENABLE_DEBUGGING OR ENABLE_DEBUGGING2)
    message(INFO ": -DDEBUGGING enabled for ${PROJECT_NAME}")
    add_compile_definitions("DEBUGGING")
endif()

message(INFO ": Building SDL app for Desktop!")
set(MAZE_BUILDER_LIB "maze_builder_lib")
add_library(${MAZE_BUILDER_LIB} STATIC ${MAZE_ALGO_FILES})
add_executable(${MAZE_BUILDER_EXE} ${DEAR_IMGUI_SRCS} ${DEPENDENCY_FILES} ${CRAFT_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/sources/main.cpp)

target_compile_features(${MAZE_BUILDER_LIB} PUBLIC cxx_std_17)

if (WIN32)
    target_include_directories(${MAZE_BUILDER_EXE} PRIVATE "C:\\Program Files (x86)\\SDL3\\include")
    include_directories("C:\\Program Files (x86)\\SDL3\\include")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui)

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

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

if(APPLE)
    target_link_libraries(${MAZE_BUILDER_EXE} OpenGL::GL Threads::Threads SDL3::SDL3 ${MAZE_BUILDER_LIB})
endif()

if(UNIX)
    target_link_libraries(${MAZE_BUILDER_EXE} dl OpenGL::GL Threads::Threads SDL3::SDL3 ${MAZE_BUILDER_LIB})
endif()

if(MINGW OR WIN32)
    target_link_libraries(${MAZE_BUILDER_EXE} ws2_32.lib OpenGL::GL Threads::Threads SDL3::SDL3 ${MAZE_BUILDER_LIB})
endif()

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/textures" DESTINATION "${CMAKE_BINARY_DIR}")

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

    file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp)

    # These tests can use the Catch2-provided main
    add_executable(${MAZE_BUILDER_TEST_EXE} ${TEST_FILES})
    target_compile_features(${MAZE_BUILDER_TEST_EXE} PUBLIC cxx_std_17)
    target_link_libraries(${MAZE_BUILDER_TEST_EXE} PRIVATE ${MAZE_BUILDER_LIB} Threads::Threads Catch2::Catch2WithMain)
    target_include_directories(${MAZE_BUILDER_TEST_EXE} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/sources")
    include(CTest)
    include(Catch)
    catch_discover_tests(${MAZE_BUILDER_TEST_EXE})
endif()
