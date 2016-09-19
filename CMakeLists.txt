cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

## Project info
project(Blowtorch CXX C)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
set(CMAKE_BUILD_TYPE_INIT "RelWithDebInfo")
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON)

## Compiling options
#set(USE_STATIC_LIBS OFF CACHE BOOL "Link against static libraries.")
#set(USE_STATIC_FLAG OFF CACHE BOOL "Use -static flag to create completely static binaries.")
#set(EXTRA_DEFS CACHE STRING "Extra Compiler Definations")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

## AppleClang doesn't know how to do these things until CMake 3.2
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang" AND
  "${CMAKE_VERSION}" VERSION_LESS 3.2)
  set(CMAKE_CXX11_STANDARD_COMPILE_OPTION "-std=c++11")
  set(CMAKE_CXX11_EXTENSION_COMPILE_OPTION "-std=gnu++11")
endif()

## Guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

## Guard against bad build-type strings
if(CMAKE_BUILD_TYPE)
  string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_tolower)
  if(    NOT cmake_build_type_tolower STREQUAL "debug"
     AND NOT cmake_build_type_tolower STREQUAL "release"
     AND NOT cmake_build_type_tolower STREQUAL "relwithdebinfo"
     AND NOT cmake_build_type_tolower STREQUAL "none")
    message(FATAL_ERROR "Unsupported build type \"${CMAKE_BUILD_TYPE}\". Allowed values are Debug, Release, RelWithDebInfo, and None (case-insensitive).")
  endif()
  message("CMake Build Type: \"${CMAKE_BUILD_TYPE}\"")
endif()

## RPATH -- See http://www.itk.org/Wiki/CMake_RPATH_handling
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}" is_system_dir)
if("${is_system_dir}" STREQUAL "-1")
	set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}")
endif("${is_system_dir}" STREQUAL "-1")

# System libraries automatically found by CMake 
find_package(Threads REQUIRED)
find_package(OpenGL 4.5 REQUIRED)
find_package(X11 REQUIRED)
find_package(Freetype 2.0.0 REQUIRED)

# User-installed libraries
find_package(GLM 0.9.7 REQUIRED)
message(STATUS "GLM found = ${GLM_FOUND} , at ${GLM_INCLUDE_DIR}")

find_package(SDL2 2.0.0 REQUIRED)
message(STATUS "SDL2 found = ${SDL2_FOUND} , at ${SDL2_INCLUDE_DIR}")

find_package(SDL2Mixer REQUIRED)
message(STATUS "SDL2 Mixer found = ${SDL2_MIXER_FOUND}, at ${SDL2_MIXER_INCLUDE_DIR}")

set(BLOWTORCH_INCLUDES ${GLM_INCLUDE_DIR} ${SDL2_INCLUDE_DIR} ${SDL2_MIXER_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS})
include_directories(${BLOWTORCH_INCLUDES})

set(BLOWTORCH_LIBS ${SDL2_LIBRARY} ${SDL2_MIXER_LIBRARY} ${X11_X11_LIB} ${OPENGL_gl_LIBRARY} ${FREETYPE_LIBRARIES})

add_subdirectory(src)

file(COPY "${CMAKE_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}")
