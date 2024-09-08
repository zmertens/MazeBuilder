cmake_minimum_required(VERSION 3.2...3.5)

# Generators using Ninja can tell if the build files are generated as a side effect of running the command, which can help avoid unnecessary rebuilds.
cmake_policy(SET CMP0058 NEW)
# Prefers to use GLVND (OpenGL Vendor Neutral Dispatch) libraries for OpenGL.
cmake_policy(SET CMP0072 NEW)

# The following thread-related commands must be before project()
if(WIN32)
  # Use native Win32 threads on Windows
  set(CMAKE_USE_WIN32_THREADS_INIT ON)
  set(CMAKE_USE_PTHREADS_INIT OFF)
else()
  # Use pthreads on Unix-like systems
  set(CMAKE_THREAD_LIBS_INIT "-pthread")
  set(CMAKE_HAVE_THREADS_LIBRARY 1)
  set(CMAKE_USE_WIN32_THREADS_INIT OFF)
  set(CMAKE_USE_PTHREADS_INIT ON)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()

project(maze_builder)

set(MAZE_BUILDER_TESTS_OUTFILE "maze_builder_tests")
set(MAZE_BUILDER_LIB "libmaze_builder")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_MAZE_TESTS "Build ${PROJECT_NAME} tests" OFF)

set(NOISE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/deps/noise/noise.c)
set(SQLITE3_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/deps/sqlite/sqlite3.c)

set(LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/args_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/sidewinder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/binary_tree.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/sidewinder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/grid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/cell.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/writer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/maze_factory.cpp)

set(CRAFT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/craft.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/db.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/item.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/ring.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/world.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/maze_thread_safe.cpp)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(INFO ": ${CMAKE_BUILD_TYPE} - enabled ${PROJECT_NAME} to use -DMAZE_DEBUG flag.")
    add_compile_definitions("MAZE_DEBUG")
endif()

add_library(${MAZE_BUILDER_LIB} STATIC ${LIB_SOURCES})
target_compile_features(${MAZE_BUILDER_LIB} PUBLIC cxx_std_17)

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/sources/main.cpp ${NOISE_SOURCES} ${SQLITE3_SOURCES} ${CRAFT_SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/emscripten_local)

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
    message(INFO ": Building ${PROJECT_NAME} app for Web!")

    # Check if CMAKE_TOOLCHAIN_FILE is set, throw a fatal error if not
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE is not set. Please specify the path to the Emscripten toolchain file.")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -matomics -mbulk-memory")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -matomics -mbulk-memory")

    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html" LINK_FLAGS "-lembind")
    # SDL3 isn't available as an Emscripten port (yet), so fetch it
    message(INFO ": Getting SDL via FetchContent")
    fetch_lib_from_git(SDL main https://github.com/libsdl-org/SDL.git)
    set(SDL_SHARED FALSE CACHE BOOL "Build a SDL shared library (if available)")
    set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")

    set_property(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_LIB} raylib)
    
    target_link_options(${PROJECT_NAME} PRIVATE
    	"SHELL:-s PTHREAD_POOL_SIZE=6 -s PTHREAD_POOL_SIZE_STRICT=2 -s WASM=1 -s USE_PTHREADS=1 -s ALLOW_MEMORY_GROWTH=1 -s STACK_SIZE=323232"
        "SHELL:-s FULL_ES3=1 -s MAX_WEBGL_VERSION=2 -s MODULARIZE=1 -s EXPORT_ES6=1"
        "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/deps/emscripten_local/shell_minimal.html"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/es/block_vertex.es.glsl@/shaders/es/block_vertex.es.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/es/block_fragment.es.glsl@/shaders/es/block_fragment.es.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/es/line_vertex.es.glsl@/shaders/es/line_vertex.es.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/es/line_fragment.es.glsl@/shaders/es/line_fragment.es.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/es/sky_vertex.es.glsl@/shaders/es/sky_vertex.es.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/es/sky_fragment.es.glsl@/shaders/es/sky_fragment.es.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/es/text_vertex.es.glsl@/shaders/es/text_vertex.es.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders/es/text_fragment.es.glsl@/shaders/es/text_fragment.es.glsl"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/textures/texture.png@/textures/texture.png"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/textures/font.png@/textures/font.png"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/textures/sign.png@/textures/sign.png"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/textures/sky.png@/textures/sky.png"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/textures/maze_in_green_32x32.bmp@/textures/maze_in_green_32x32.bmp")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        target_link_options(${PROJECT_NAME} PRIVATE "SHELL:-s STACK_OVERFLOW_CHECK=2 -s SAFE_HEAP=2 -s ASSERTIONS=2 -gsource-map")
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_link_options(${PROJECT_NAME} PRIVATE "SHELL:-O3")
    endif()
else()
    # Download raylib
    fetch_lib_from_git(raylib "5.0" https://github.com/raysan5/raylib)    
    set_property(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_deps/raylib-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
    if(UNIX)
        message(INFO ": Building ${PROJECT_NAME} app for Desktop UNIX!")
    elseif(APPLE)
        message(INFO ": Building ${PROJECT_NAME} app for Desktop APPLE!")
    elseif(WIN32 OR MINGW)
        message(INFO ": Building ${PROJECT_NAME} app for Desktop WINDOWS!")
    endif()

    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_LIB} raylib)
endif()

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/textures" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/secure_http_server.py" DESTINATION "${CMAKE_BINARY_DIR}")

if(BUILD_MAZE_TESTS)
    find_package(Catch2 3 QUIET)
    if(CATCH2_FOUND)
        message(INFO ": Getting Catch2 via find_package")
    endif()
    if(NOT CATCH2_FOUND)
        message(INFO ": Getting Catch2 via FetchContent")
        fetch_lib_from_git(Catch2 v3.4.0 https://github.com/catchorg/Catch2.git)
        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    endif()

    enable_testing()

    file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp)

    # These tests can use the Catch2-provided main
    add_executable(${MAZE_BUILDER_TESTS_OUTFILE} ${TEST_FILES})
    add_test(NAME RUN_MAZE_TESTS COMMAND ${MAZE_BUILDER_TESTS_OUTFILE})
    target_compile_features(${MAZE_BUILDER_TESTS_OUTFILE} PUBLIC cxx_std_17)
    target_link_libraries(${MAZE_BUILDER_TESTS_OUTFILE} PRIVATE ${MAZE_BUILDER_LIB} Catch2::Catch2WithMain)
    target_include_directories(${MAZE_BUILDER_TESTS_OUTFILE} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/sources")
endif()

