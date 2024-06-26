cmake_minimum_required(VERSION 3.2...3.5)

cmake_policy(SET CMP0058 NEW)
cmake_policy(SET CMP0072 NEW)

# The following thread-related commands must be before project()
set(CMAKE_THREAD_LIBS_INIT "-pthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

project(maze_builder)

set(MAZE_BUILDER_OUTFILE "maze_builder")
set(MAZE_BUILDER_TESTS_OUTFILE "maze_builder_tests")
set(MAZE_BUILDER_LIB "maze_builder_lib")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -matomics -mbulk-memory")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -matomics -mbulk-memory")

option(BUILD_TESTS "Build ${PROJECT_NAME} tests" OFF)

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

set(GLAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/src/glad_3_0.c)
set(NOISE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/deps/noise/noise.c)
set(SQLITE3_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/deps/sqlite/sqlite3.c)
set(TINY_C_THREAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/deps/tinycthread/tinycthread.c)

set(MAZE_BUILDER_LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/args_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/sidewinder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/binary_tree.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/sidewinder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/grid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/cell.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/writer.cpp)

set(CRAFT_SOURCES
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

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(INFO ": ${CMAKE_BUILD_TYPE} - enabled ${PROJECT_NAME} to use -DMAZE_DEBUG flag.")
    add_compile_definitions("MAZE_DEBUG")
endif()

add_library(${MAZE_BUILDER_LIB} STATIC ${MAZE_BUILDER_LIB_SOURCES})

target_compile_features(${MAZE_BUILDER_LIB} PUBLIC cxx_std_17)

add_executable(${MAZE_BUILDER_OUTFILE} ${DEAR_IMGUI_SRCS} ${GLAD_SOURCES} ${TINY_C_THREAD_SOURCES} ${NOISE_SOURCES} ${SQLITE3_SOURCES} ${CRAFT_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/sources/main.cpp)
target_include_directories(${MAZE_BUILDER_OUTFILE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/include)
target_include_directories(${MAZE_BUILDER_OUTFILE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps)
target_include_directories(${MAZE_BUILDER_OUTFILE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/emscripten)
target_include_directories(${MAZE_BUILDER_OUTFILE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/dearimgui)

include(FetchContent)
function(fetch_lib_from_git name_of_lib tag git_repo)
    FetchContent_Declare(
        ${name_of_lib}
        GIT_REPOSITORY ${git_repo}
        GIT_TAG ${tag}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(${name_of_lib})
endfunction()

if(EMSCRIPTEN)
    message(INFO ": Building SDL app for Web!")

    # Check if CMAKE_TOOLCHAIN_FILE is set, throw a fatal error if not
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE is not set. Please specify the path to the Emscripten toolchain file.")
    endif()

    set_target_properties(${MAZE_BUILDER_OUTFILE} PROPERTIES SUFFIX ".html")
    # SDL3 isn't available as an Emscripten port (yet), so fetch it
    message(INFO ": Getting SDL via FetchContent")
    fetch_lib_from_git(SDL main https://github.com/libsdl-org/SDL.git)
    set(SDL_SHARED FALSE CACHE BOOL "Build a SDL shared library (if available)")
    set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")
    set_property(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
    
    target_link_libraries(${MAZE_BUILDER_OUTFILE} SDL3::SDL3 ${MAZE_BUILDER_LIB})
    
    target_link_options(${MAZE_BUILDER_OUTFILE} PRIVATE
    	"SHELL:-s PTHREAD_POOL_SIZE=5 -s PTHREAD_POOL_SIZE_STRICT=2 -s WASM=1 -gsource-map"
    	"SHELL:-s FULL_ES3=1 -s MIN_WEBGL_VERSION=2 -s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=1 -s USE_PTHREADS=1 -s ALLOW_MEMORY_GROWTH=1"
        "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/deps/emscripten/shell_minimal.html"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/block_vertex.glsl@/shaders/block_vertex.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/block_fragment.glsl@/shaders/block_fragment.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line_vertex.glsl@/shaders/line_vertex.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/line_fragment.glsl@/shaders/line_fragment.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sky_vertex.glsl@/shaders/sky_vertex.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sky_fragment.glsl@/shaders/sky_fragment.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/text_vertex.glsl@/shaders/text_vertex.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/text_fragment.glsl@/shaders/text_fragment.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/textures/sign.png@/textures/sign.png"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/textures/sky.png@/textures/sky.png"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/textures/texture.png@/textures/texture.png"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/textures/font.png@/textures/font.png"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/textures/maze_in_green_32x32.bmp@/textures/maze_in_green_32x32.bmp")
endif()

if(APPLE AND NOT EMSCRIPTEN)
    target_link_libraries(${MAZE_BUILDER_OUTFILE} OpenGL::GL SDL3::SDL3 ${MAZE_BUILDER_LIB})
endif()

if(UNIX AND NOT EMSCRIPTEN)
    message(INFO ": Building SDL app for Desktop UNIX!")
    
    find_package(OpenGL REQUIRED)

    # Find/Configure SDL
    find_package(SDL3 QUIET)
    if(SDL3_FOUND)
        message(INFO ": Getting SDL via find_package")
    endif()

    # If all else fails, download SDL
    if(NOT SDL3_FOUND)
        # Call custom function to fetch from git
        fetch_lib_from_git(SDL main https://github.com/libsdl-org/SDL.git)
        set_property(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif()

    target_link_libraries(${MAZE_BUILDER_OUTFILE} dl OpenGL::GL SDL3::SDL3 ${MAZE_BUILDER_LIB})
endif()

if(MINGW OR WIN32)
    message(INFO ": Building SDL app for Desktop WINDOWS!")
        
    find_package(OpenGL REQUIRED)

    target_include_directories(${MAZE_BUILDER_OUTFILE} PRIVATE "C:\\Program Files (x86)\\SDL3\\include")

    # Find/Configure SDL
    find_package(SDL3 QUIET)
    if(SDL3_FOUND)
        message(INFO ": Getting SDL via find_package")
    endif()

    # If all else fails, download SDL
    if(NOT SDL3_FOUND)
        # Call custom function to fetch from git
        fetch_lib_from_git(SDL main https://github.com/libsdl-org/SDL.git)
        set_property(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif()
    
    target_link_libraries(${MAZE_BUILDER_OUTFILE} ws2_32.lib OpenGL::GL SDL3::SDL3 ${MAZE_BUILDER_LIB})
endif()

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/textures" DESTINATION "${CMAKE_BINARY_DIR}")

if(BUILD_TESTS)
    find_package(Catch2 3 QUIET)
    if(CATCH2_FOUND)
        message(INFO ": Getting Catch2 via find_package")
    endif()
    if(NOT CATCH2_FOUND)
        message(INFO ": Getting Catch2 via FetchContent")
        fetch_lib_from_git(Catch2 v3.4.0 https://github.com/catchorg/Catch2.git)
        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    endif()

    file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp)

    # These tests can use the Catch2-provided main
    add_executable(${MAZE_BUILDER_TESTS_OUTFILE} ${TEST_FILES})
    target_compile_features(${MAZE_BUILDER_TESTS_OUTFILE} PUBLIC cxx_std_17)
    target_link_libraries(${MAZE_BUILDER_TESTS_OUTFILE} PRIVATE ${MAZE_BUILDER_LIB} Catch2::Catch2WithMain)
    target_include_directories(${MAZE_BUILDER_TESTS_OUTFILE} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/sources")
endif()

