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

project(maze_builder VERSION 5.1.5 LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(NoInSourceBuilds)
include(CTest)
include(Packaging)

set(MAZE_BUILDER_LIB "maze_builder_lib")

add_subdirectory(sources)
add_subdirectory(tests)
add_subdirectory(examples bin)
