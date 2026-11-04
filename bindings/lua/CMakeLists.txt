message(STATUS "Building Lua binding")

# Vanilla Lua, wrapped with CMake support
set(LUA_ENABLE_SHARED OFF CACHE BOOL "" FORCE)
set(LUA_BUILD_COMPILER OFF CACHE BOOL "" FORCE)
set(LUA_SUPPORT_DL ON CACHE BOOL "" FORCE)
set(LUA_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(FetchGitRepo)
FetchGitRepo(lua_cmake https://github.com/walterschell/Lua.git v5.4.7)

# Loadable module: require "maze"
add_library(maze_lua MODULE maze_lua.cpp)
set_target_properties(maze_lua PROPERTIES OUTPUT_NAME maze PREFIX "" POSITION_INDEPENDENT_CODE ON)
target_compile_features(maze_lua PRIVATE cxx_std_20)
target_link_libraries(maze_lua PRIVATE MazeBuilder::MazeBuilder lua_static)

# Standalone interpreter with "maze" preregistered
add_executable(mazebuilder_lua main.cpp maze_lua.cpp)
target_compile_features(mazebuilder_lua PRIVATE cxx_std_20)
target_link_libraries(mazebuilder_lua PRIVATE MazeBuilder::MazeBuilder lua_static)

if(MAZE_BUILDER_TESTS)
    enable_testing()
    add_test(NAME RUN_LUA_BINDING_EXE
        COMMAND mazebuilder_lua ${CMAKE_CURRENT_SOURCE_DIR}/examples/hello.lua)
    add_test(NAME RUN_LUA_BINDING_MODULE
        COMMAND mazebuilder_lua ${CMAKE_CURRENT_SOURCE_DIR}/examples/require_module.lua)
    set_tests_properties(RUN_LUA_BINDING_MODULE PROPERTIES
        ENVIRONMENT "MAZE_MODULE_DIR=$<TARGET_FILE_DIR:maze_lua>")
endif()
