
# Set the name of the executable
set(MAZE_BUILDER_EXE "cli")
add_executable(${MAZE_BUILDER_EXE} main.cpp)
target_link_libraries(${MAZE_BUILDER_EXE} ${MAZE_BUILDER_LIB}_static)
target_include_directories(${MAZE_BUILDER_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/include)
