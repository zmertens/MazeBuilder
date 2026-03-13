# MazePathtracer – 3D maze path-tracing demo using SDL3 GPU API
#
# Dependencies:
#   - MazeBuilder core library (maze generation)
#   - SDL3 (window management and GPU-based rendering)
#   - C++ threads

set(EXE_NAME "mazebuilderpathtracer")

set(EXE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pathtracer_app.cpp)

add_executable(${EXE_NAME} ${EXE_SOURCES})

target_compile_features(${EXE_NAME} PRIVATE cxx_std_20)

target_compile_definitions(${EXE_NAME} PRIVATE
    "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:${MAZE_DEBUG_DEF}>")

target_include_directories(${EXE_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/deps>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(${EXE_NAME} PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    ${MAZE_BUILDER_CORE_LIB}_static
    SDL3::SDL3)

message(INFO ": Building ${EXE_NAME} for the Desktop!")

# Copy shared/dlls on Windows only
if(WIN32)
    add_custom_command(
        TARGET ${EXE_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
            $<TARGET_FILE:SDL3::SDL3-shared>
            $<TARGET_FILE_DIR:${EXE_NAME}>
        VERBATIM)
endif()