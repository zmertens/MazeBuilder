include(BuildInfo)

set(CPP_BASE64_SRCS ${CMAKE_SOURCE_DIR}/deps/cpp-base64/base64.cpp)

# Don't include this source file in Visual Studio projects
set_source_files_properties(${CPP_BASE64_SRCS} PROPERTIES 
    VS_TOOL_OVERRIDE "None"
    HEADER_FILE_ONLY FALSE
)

set(MAZE_BUILDER_CORE_SRCS
    args.cpp
    base64_helper.cpp
    binary_tree.cpp
    cell.cpp
    colored_grid.cpp
    dfs.cpp
    distance_grid.cpp
    distances.cpp
    grid.cpp
    grid_factory.cpp
    json_helper.cpp
    lab.cpp
    maze_adapter.cpp
    objectify.cpp
    pixels.cpp
    randomizer.cpp
    sidewinder.cpp
    string_view_utils.cpp
    stringify.cpp
    wavefront_object_helper.cpp
    writer.cpp)

if(MAZE_BUILDER_COVERAGE)
    message(STATUS "Building ${PROJECT_NAME} with code coverage")
    
    # Include CppCheck module first
    include(CppCheck)
    
    # Find cppcheck executable
    find_program(CPPCHECK_EXECUTABLE cppcheck)
    if(NOT CPPCHECK_EXECUTABLE)
        message(WARNING "cppcheck not found, static analysis will be skipped")
    else()
        message(STATUS "Found CppCheck: ${CPPCHECK_EXECUTABLE}")
        
        # Create the custom target with proper executable reference
        set(RUN_CPPCHECK "run_cppcheck")
        add_custom_target(${RUN_CPPCHECK}
            COMMAND ${CPPCHECK_EXECUTABLE} --enable=all --inconclusive --quiet --xml --xml-version=2
            --output-file=cppcheck.xml ${MAZE_BUILDER_CORE_SRCS}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running CppCheck on ${PROJECT_NAME} sources"
        )
        
        # No need to call AddCppCheck on the custom target
        # The AddCppCheck function is designed for actual library/executable targets
        # AddCppCheck(${MAZE_BUILDER_LIB_OUT}_obj)
    endif()

    list(APPEND BASE_COMPILE_FLAGS "$<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:--coverage>")

endif()

list(APPEND BASE_COMPILE_FLAGS "$<$<CXX_COMPILER_ID:Emscripten>:-matomics;-mbulk-memory>")
list(APPEND BASE_COMPILE_FLAGS "$<$<CXX_COMPILER_ID:MSVC>:/W4;/WX>")

# Setup object library
set(MAZE_BUILDER_LIB_OUT "mazebuildercore")
add_library(${MAZE_BUILDER_LIB_OUT}_obj OBJECT ${MAZE_BUILDER_CORE_SRCS} ${CPP_BASE64_SRCS})
target_include_directories(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE $<INSTALL_INTERFACE:deps> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps>)
# Add STB as a system include to suppress warnings
target_include_directories(${MAZE_BUILDER_LIB_OUT}_obj SYSTEM PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps/stb>)
target_compile_features(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE cxx_std_20)
target_compile_options(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE ${BASE_COMPILE_FLAGS} "-pthread")

# Generate the version info for the library
BuildInfo(${MAZE_BUILDER_LIB_OUT}_obj)

target_include_directories(${MAZE_BUILDER_LIB_OUT}_obj PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
set_target_properties(${MAZE_BUILDER_LIB_OUT}_obj PROPERTIES POSITION_INDEPENDENT_CODE 1)
target_compile_definitions(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:MAZE_DEBUG>")

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
