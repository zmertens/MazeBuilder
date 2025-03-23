include(BuildInfo)

set(CPP_BASE64_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../deps/cpp-base64/base64.cpp)

set(MAZE_BUILDER_CORE_SRCS
    args.cpp
    base64_helper.cpp
    binary_tree.cpp
    cell.cpp
    colored_grid.cpp
    dfs.cpp
    distance_grid.cpp
    distances.cpp
    factory.cpp
    grid.cpp
    json_helper.cpp
    lab.cpp
    maze.cpp
    #randomizer.cpp
    sidewinder.cpp
    stringz.cpp
    wavefront_object_helper.cpp
    writer.cpp)

# Use these features on the Web only
if(EMSCRIPTEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -matomics -mbulk-memory")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -matomics -mbulk-memory")
endif()

if(MAZE_BUILDER_COVERAGE)
    message(INFO ": Building ${PROJECT_NAME} with code coverage")
    set(RUN_CPPCHECK "run_cppcheck")
    include(CppCheck)
    message(INFO ": Found CppCheck: ${CPPCHECK_EXECUTABLE}")
    add_custom_target(${RUN_CPPCHECK}
        COMMAND ${CPPCHECK_EXECUTABLE} --enable=all --inconclusive --quiet --xml --xml-version=2
        --output-file=cppcheck.xml ${MAZE_BUILDER_CORE_SRCS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running CppCheck on ${PROJECT_NAME} sources"
    )
    AddCppCheck(${RUN_CPPCHECK})
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    endif()
endif()

# Setup object library
set(MAZE_BUILDER_LIB_OUT "mazebuildercore")
add_library(${MAZE_BUILDER_LIB_OUT}_obj OBJECT ${MAZE_BUILDER_CORE_SRCS} ${CPP_BASE64_SRCS})
target_include_directories(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../deps)
target_compile_features(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE cxx_std_17)

# Generate the version info for the library
BuildInfo(${MAZE_BUILDER_LIB_OUT}_obj)

file(GLOB MAZE_BUILDER_HEADERS "${CMAKE_SOURCE_DIR}/include/MazeBuilder/*.h")
# Requires CMake 3.28
#target_sources(${MAZE_BUILDER_LIB_OUT}_obj PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_SOURCE_DIR}/include" FILES ${MAZE_BUILDER_HEADERS})
target_include_directories(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set_target_properties(${MAZE_BUILDER_LIB_OUT}_obj PROPERTIES POSITION_INDEPENDENT_CODE 1)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(INFO ": ${CMAKE_BUILD_TYPE} - enabled ${PROJECT_NAME} to use -DMAZE_DEBUG flag.")
    target_compile_definitions(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE "MAZE_DEBUG")
endif()

# Shared build
if(NOT EMSCRIPTEN)
    add_library(${MAZE_BUILDER_CORE_LIB}_shared SHARED)
    target_link_libraries(${MAZE_BUILDER_CORE_LIB}_shared ${MAZE_BUILDER_LIB_OUT}_obj)
endif()

# Static build
add_library(${MAZE_BUILDER_CORE_LIB}_static STATIC)
target_link_libraries(${MAZE_BUILDER_CORE_LIB}_static ${MAZE_BUILDER_LIB_OUT}_obj)

# Installation
include(GNUInstallDirs)
#install(TARGETS ${MAZE_BUILDER_LIB_OUT} ${MAZE_BUILDER_CORE_LIB}_shared ${MAZE_BUILDER_CORE_LIB}_static
#  EXPORT MazeBuilderLibrary
#  ARCHIVE COMPONENT development
#  LIBRARY COMPONENT runtime
#  FILE_SET HEADERS COMPONENT runtime
#)

#if (UNIX)
#    install(CODE "execute_process(COMMAND ldconfig)" COMPONENT runtime)
#endif()

#install(EXPORT MazeBuilderLibrary
#  DESTINATION ${CMAKE_INSTALL_LIBDIR}/MazeBuilder/cmake
#  NAMESPACE MazeBuilder::
#  COMPONENT runtime
#)

#install(FILES "MazeBuilderConfig.cmake"
#  DESTINATION ${CMAKE_INSTALL_LIBDIR}/MazeBuilder/cmake
#)
