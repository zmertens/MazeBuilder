include(CMakeFindDependencyMacro)
find_dependency(Threads)
find_dependency(fmt)

include("${CMAKE_CURRENT_LIST_DIR}/MazeBuilderLibrary.cmake")
