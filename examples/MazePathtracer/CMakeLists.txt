# MazePathtracer – 3D maze path-tracing demo using SDL3 GPU API
#
# Dependencies:
#   - MazeBuilder core library (maze generation)
#   - SDL3 (window management and GPU-based rendering)
#   - C++ threads (for randomizer)

set(EXE_NAME "mazepathtracer")

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

if(EMSCRIPTEN)
    message(INFO ": Building ${EXE_NAME} for the Web!")

    set_target_properties(${EXE_NAME} PROPERTIES
        SUFFIX ".html"
        LINK_FLAGS "-lembind")

    set(EMSCRIPTEN_DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../deps/emscripten_local)
    target_include_directories(${EXE_NAME} PRIVATE
        $<INSTALL_INTERFACE:deps/emscripten_local>
        $<BUILD_INTERFACE:${EMSCRIPTEN_DEP_PATH}>)

    target_link_options(${EXE_NAME} PRIVATE
        "SHELL:-s WASM=1 -s MODULARIZE=1 -s EXPORT_ES6=1"
        "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/emscripten_local/shell_minimal.html")

    target_link_options(${EXE_NAME} PRIVATE
        "$<$<CONFIG:Debug>:SHELL:-s STACK_OVERFLOW_CHECK=2 -s ASSERTIONS=2 -gsource-map --profile>")
    target_link_options(${EXE_NAME} PRIVATE
        "$<$<CONFIG:Release>:SHELL:-O3>")

else()
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

endif(EMSCRIPTEN)
