include(BuildInfo)

set(CPP_BASE64_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../deps/cpp-base64/base64.cpp)

message(INFO ": Configuring ${PROJECT_NAME} v${MAZE_BUILDER_VERSION}")

set(MAZE_BUILDER_CORE_SRCS
    args.cpp
    async_logger.cpp
    bytes.cpp
    bt_maze_create_state.cpp
    dfs_maze_create_state.cpp
    cell.cpp
    colored_grid.cpp
    distance_grid.cpp
    distances.cpp
    grid.cpp
    io_utils.cpp
    json_helper.cpp
    lab.cpp
    link_maze_and_create_state.cpp
    loading_state.cpp
    mask.cpp
    masked_grid.cpp
    masked_maze_and_create_state.cpp
    parsing_state.cpp
    pixels_create_state.cpp
    prims_maze_create_state.cpp
    randomizer.cpp
    runtime_app.cpp
    runtime_stack.cpp
    state.cpp
    stringify_create_state.cpp
    string_utils.cpp    
    sw_maze_create_state.cpp
    topology.cpp
    wavefront_object_create_state.cpp
    write_to_output_state.cpp)

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

# Set position independent code globally for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Dependencies
include(FetchContent)
message(INFO ": Fetching fmt library...")
# fmt version 12.1.0
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 407c905e45ad75fc29bf0f9bb7c5c2fd3475976f)
# Configure fmt options before making it available
set(FMT_INSTALL ON CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)

set(_fmt_source_dir "${CMAKE_BINARY_DIR}/_deps/fmt-src")
if(EXISTS "${_fmt_source_dir}" AND NOT EXISTS "${_fmt_source_dir}/CMakeLists.txt")
    message(STATUS "Detected stale empty fmt cache, forcing FetchContent re-clone")
    file(REMOVE_RECURSE "${_fmt_source_dir}")
    file(REMOVE
        "${CMAKE_BINARY_DIR}/CMakeFiles/fc-stamp/fmt/fmt-gitclone-lastrun.txt"
        "${CMAKE_BINARY_DIR}/CMakeFiles/fc-stamp/fmt/download.stamp"
        "${CMAKE_BINARY_DIR}/CMakeFiles/fc-stamp/fmt/update.stamp"
        "${CMAKE_BINARY_DIR}/CMakeFiles/fc-stamp/fmt/patch.stamp")
endif()

FetchContent_MakeAvailable(fmt)

# Normalize fmt target name across package variants.
if(NOT TARGET fmt::fmt)
    if(TARGET fmt)
        add_library(fmt::fmt ALIAS fmt)
    elseif(TARGET fmt-header-only)
        add_library(fmt::fmt ALIAS fmt-header-only)
    else()
        message(FATAL_ERROR "fmt dependency target was not created. If using an existing build directory, clear stale fmt cache under _deps (fmt-src/fmt-build/fmt-tmp) and reconfigure.")
    endif()
endif()

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

set(_maze_builder_install_targets ${MAZE_BUILDER_LIB_OUT}_obj ${MAZE_BUILDER_LIB_OUT}_static)
if(NOT EMSCRIPTEN)
    list(APPEND _maze_builder_install_targets ${MAZE_BUILDER_LIB_OUT}_shared)
endif()

install(TARGETS ${_maze_builder_install_targets}
  EXPORT MazeBuilderLibrary
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT development
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtime
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT runtime
  OBJECTS DESTINATION ${CMAKE_INSTALL_LIBDIR}/objects COMPONENT development
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/MazeBuilder
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  COMPONENT runtime
)

if (UNIX)
    install(CODE "execute_process(COMMAND ldconfig)" COMPONENT runtime)
endif()

install(EXPORT MazeBuilderLibrary
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/MazeBuilder/cmake
  NAMESPACE MazeBuilder::
  COMPONENT runtime
)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/MazeBuilderConfig.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/MazeBuilder/cmake
  COMPONENT runtime
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/MazeBuilderConfigVersion.cmake"
  VERSION ${MAZE_BUILDER_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/MazeBuilderConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/MazeBuilder/cmake
  COMPONENT runtime
)
