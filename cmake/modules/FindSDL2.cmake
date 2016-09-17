# Locate the SDL2 library
#
# This module defines the following variables:
#
# SDL2_LIBRARY      the name of the library;
# SDL2_INCLUDE_DIR  where to find glfw include files.
# SDL2_FOUND        true if both the SDL2_LIBRARY and SDL2_INCLUDE_DIR have been found.
#
# To help locate the library and include file, you can define a 
# variable called SDL2_ROOT which points to the root of the SDL library 
# installation.
#

# default search dirs
set( _SDL2_HEADER_SEARCH_DIRS 
  "/usr/include"
  "/usr/local/include"
  "C:/Program Files (x86)/SDL2/include" )
set( _SDL2_LIB_SEARCH_DIRS
  "/usr/lib"
  "/usr/local/lib"
  "C:/Program Files (x86)/SDL2/lib-msvc110" )

# Check environment for root search directory
set( _SDL2_ENV_ROOT $ENV{SDL2_ROOT} )
if( NOT SDL2_ROOT AND _SDL2_ENV_ROOT )
  set(SDL2_ROOT ${_SDL2_ENV_ROOT} )
endif()

# Put user specified location at beginning of search
if( SDL2_ROOT )
  list( INSERT _SDL2_HEADER_SEARCH_DIRS 0 "${SDL2_ROOT}/include" )
  list( INSERT _SDL2_LIB_SEARCH_DIRS 0 "${SDL2_ROOT}/lib" )
endif()

# Search for the header 
FIND_PATH(SDL2_INCLUDE_DIR "SDL2"
  PATHS ${_SDL2_HEADER_SEARCH_DIRS} )

# Search for the library
FIND_LIBRARY(SDL2_LIBRARY NAMES SDL2
  PATHS ${_SDL2_LIB_SEARCH_DIRS} )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2 DEFAULT_MSG
                                  SDL2_LIBRARY SDL2_INCLUDE_DIR)