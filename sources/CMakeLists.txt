include(CppCheck)
include(Doxygen)

set(CPP_BASE64_SRCS ${CMAKE_SOURCE_DIR}/deps/cpp-base64/base64.cpp)

set(MAZE_BUILDER_LIB_SOURCES
    args_builder.cpp
    sidewinder.cpp
    binary_tree.cpp
    sidewinder.cpp
    grid.cpp
    cell.cpp
    writer.cpp
    maze_builder.cpp
    maze_factory.cpp
    distances.cpp
    distance_grid.cpp
    colored_grid.cpp
    dfs.cpp)

# Use these features on the Web only
if(EMSCRIPTEN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -matomics -mbulk-memory")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -matomics -mbulk-memory")
endif()

# Setup object library
add_library(${MAZE_BUILDER_LIB}_obj OBJECT ${MAZE_BUILDER_LIB_SOURCES} ${CPP_BASE64_SRCS})
target_include_directories(${MAZE_BUILDER_LIB}_obj PRIVATE ${CMAKE_SOURCE_DIR}/deps)
target_compile_features(${MAZE_BUILDER_LIB}_obj PUBLIC cxx_std_17)
# file(GLOB MAZE_BUILDER_HEADERS ${CMAKE_SOURCE_DIR}/include/MazeBuilder/*.h)
# target_sources(${MAZE_BUILDER_LIB}_obj PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_SOURCE_DIR}/include" FILES ${MAZE_BUILDER_HEADERS})
target_include_directories(${MAZE_BUILDER_LIB}_obj PRIVATE ${CMAKE_SOURCE_DIR}/include)
set_target_properties(${MAZE_BUILDER_LIB}_obj PROPERTIES POSITION_INDEPENDENT_CODE 1)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(INFO ": ${CMAKE_BUILD_TYPE} - enabled ${PROJECT_NAME} to use -DMAZE_DEBUG flag.")
    target_compile_definitions(${MAZE_BUILDER_LIB}_obj PRIVATE "MAZE_DEBUG")
endif()

# Setup shared library
add_library(${MAZE_BUILDER_LIB}_shared SHARED)
target_link_libraries(${MAZE_BUILDER_LIB}_shared ${MAZE_BUILDER_LIB}_obj)

# Setup static library
add_library(${MAZE_BUILDER_LIB}_static STATIC)
target_link_libraries(${MAZE_BUILDER_LIB}_static ${MAZE_BUILDER_LIB}_obj)

#Doxygen(maze_builder sources)

# Installation
include(GNUInstallDirs)
install(TARGETS ${MAZE_BUILDER_LIB}_obj ${MAZE_BUILDER_LIB}_shared ${MAZE_BUILDER_LIB}_static
  EXPORT MazeBuilderLibrary
  ARCHIVE COMPONENT development
  LIBRARY COMPONENT runtime
  #FILE_SET HEADERS COMPONENT runtime
)

if (UNIX)
    install(CODE "execute_process(COMMAND ldconfig)" COMPONENT runtime)
endif()

install(EXPORT MazeBuilderLibrary
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/MazeBuilder/cmake
  NAMESPACE MazeBuilder::
  COMPONENT runtime
)

install(FILES "MazeBuilderConfig.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/MazeBuilder/cmake
)
