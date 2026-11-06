message(STATUS "Building DLL library")

# Loadable module: require "maze"
add_library(maze_dll MODULE maze_dll.cpp)
set_target_properties(maze_dll PROPERTIES OUTPUT_NAME maze PREFIX "" POSITION_INDEPENDENT_CODE ON)
target_compile_features(maze_dll PRIVATE cxx_std_20)
target_link_libraries(maze_dll PRIVATE MazeBuilder::MazeBuilder)

# Standalone interpreter with "maze" preregistered
add_executable(mazebuilder_dll_test main.cpp maze_dll.cpp)
target_compile_features(mazebuilder_dll_test PRIVATE cxx_std_20)
target_link_libraries(mazebuilder_dll_test PRIVATE MazeBuilder::MazeBuilder)
