include(BuildInfo)

set(CPP_BASE64_SRCS ${PROJECT_SOURCE_DIR}/deps/cpp-base64/base64.cpp)

# Don't include this source file in Visual Studio projects
set_source_files_properties(${CPP_BASE64_SRCS} PROPERTIES 
    VS_TOOL_OVERRIDE "None"
    HEADER_FILE_ONLY FALSE
)

message(INFO ": Configuring ${PROJECT_NAME} v${MAZE_BUILDER_VERSION}")

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
    io_utils.cpp
    json_helper.cpp
    lab.cpp
    maze_factory.cpp
    objectify.cpp
    pixels.cpp
    randomizer.cpp
    sidewinder.cpp
    stringify.cpp
    string_utils.cpp
    wavefront_object_helper.cpp)

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

list(APPEND BASE_COMPILE_FLAGS "$<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-pthread>")
list(APPEND BASE_COMPILE_FLAGS "$<$<CXX_COMPILER_ID:Emscripten>:-matomics;-mbulk-memory>")
list(APPEND BASE_COMPILE_FLAGS "$<$<CXX_COMPILER_ID:MSVC>:/W4;/WX>")

# Dependencies
message(INFO ": Fetching fmt library...")

# Set position independent code globally for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
# fmt version 11.2.0
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 40626af88bd7df9a5fb80be7b25ac85b122d6c21)
# Configure fmt options before making it available
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(fmt)

# Setup object library
set(MAZE_BUILDER_LIB_OUT ${MAZE_BUILDER_CORE_LIB})

add_library(${MAZE_BUILDER_LIB_OUT}_obj OBJECT ${MAZE_BUILDER_CORE_SRCS} ${CPP_BASE64_SRCS})
# Add compile definition to disable problematic FMT_STRING usage
target_compile_definitions(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:${MAZE_DEBUG_DEF}>")
target_compile_definitions(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE FMT_STRING_ALIAS=0)
target_compile_features(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE cxx_std_20)
target_compile_options(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE ${BASE_COMPILE_FLAGS})
target_include_directories(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE $<INSTALL_INTERFACE:deps> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps>)
# Add STB as a system include to suppress warnings
target_include_directories(${MAZE_BUILDER_LIB_OUT}_obj SYSTEM PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps/stb>)
target_include_directories(${MAZE_BUILDER_LIB_OUT}_obj PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_link_libraries(${MAZE_BUILDER_LIB_OUT}_obj PRIVATE fmt::fmt)

# Generate the version info for the library
BuildInfo(${MAZE_BUILDER_LIB_OUT}_obj)

add_library(MazeBuilder::MazeBuilder ALIAS ${MAZE_BUILDER_LIB_OUT}_obj)

# Shared build
if(NOT EMSCRIPTEN)
    add_library(${MAZE_BUILDER_LIB_OUT}_shared SHARED)
    target_link_libraries(${MAZE_BUILDER_LIB_OUT}_shared ${MAZE_BUILDER_LIB_OUT}_obj fmt::fmt)
endif()

# Static build
add_library(${MAZE_BUILDER_LIB_OUT}_static STATIC)
target_link_libraries(${MAZE_BUILDER_LIB_OUT}_static ${MAZE_BUILDER_LIB_OUT}_obj fmt::fmt)

# Installation
include(GNUInstallDirs)
#install(TARGETS ${MAZE_BUILDER_LIB_OUT} ${MAZE_BUILDER_LIB_OUT}_shared ${MAZE_BUILDER_LIB_OUT}_static
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
