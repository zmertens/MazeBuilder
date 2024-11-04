#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MazeBuilder::maze_builder_lib_shared" for configuration ""
set_property(TARGET MazeBuilder::maze_builder_lib_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(MazeBuilder::maze_builder_lib_shared PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libmaze_builder_lib_shared.so"
  IMPORTED_SONAME_NOCONFIG "libmaze_builder_lib_shared.so"
  )

list(APPEND _cmake_import_check_targets MazeBuilder::maze_builder_lib_shared )
list(APPEND _cmake_import_check_files_for_MazeBuilder::maze_builder_lib_shared "${_IMPORT_PREFIX}/lib/libmaze_builder_lib_shared.so" )

# Import target "MazeBuilder::maze_builder_lib_static" for configuration ""
set_property(TARGET MazeBuilder::maze_builder_lib_static APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(MazeBuilder::maze_builder_lib_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libmaze_builder_lib_static.a"
  )

list(APPEND _cmake_import_check_targets MazeBuilder::maze_builder_lib_static )
list(APPEND _cmake_import_check_files_for_MazeBuilder::maze_builder_lib_static "${_IMPORT_PREFIX}/lib/libmaze_builder_lib_static.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
