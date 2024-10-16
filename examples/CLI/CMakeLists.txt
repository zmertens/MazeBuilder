
# Set the name of the executable
set(MAZE_BUILDER_EXE "maze_builder")
add_executable(${MAZE_BUILDER_EXE} main.cpp)
target_link_libraries(${MAZE_BUILDER_EXE} ${MAZE_BUILDER_LIB}_static)
