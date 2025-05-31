cmake_minimum_required(VERSION 3.1...3.5)

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

project(MazeBuilder VERSION 6.3.5 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MAZE_BUILDER_CORE_LIB "mazebuildercore")

option(MAZE_BUILDER_DOCS "Build ${PROJECT_NAME} documentation" OFF)
option(MAZE_BUILDER_TESTS "Build ${PROJECT_NAME} with tests" OFF)
option(MAZE_BUILDER_COVERAGE "Build ${PROJECT_NAME} with code coverage" OFF)
option(MAZE_BUILDER_MEMCHECK "Build ${PROJECT_NAME} with memory analysis" OFF)
option(MAZE_BUILDER_EXAMPLES "Build ${PROJECT_NAME} examples" OFF)

include(NoInSourceBuilds)

add_subdirectory(sources)
if(MAZE_BUILDER_DOCS)
    add_subdirectory(docs)
endif()
if(MAZE_BUILDER_TESTS)
    add_subdirectory(tests)
endif()
if(MAZE_BUILDER_EXAMPLES)
    add_subdirectory(examples bin)
endif()
