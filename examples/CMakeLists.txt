
option(MAZE_BUILD_EXAMPLES "Build ${PROJECT_NAME} examples" OFF)
if(MAZE_BUILD_EXAMPLES)
    message(INFO ": Building ${PROJECT_NAME} examples")
    add_subdirectory(Voxels)
    add_subdirectory(CLI)
    add_subdirectory(MazeBuilderJS)
endif()