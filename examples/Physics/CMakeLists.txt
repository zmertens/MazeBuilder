# Setup dependencies
set(EXE_NAME "mazebuilderphysics")

set(DEAR_IMGUI_DEP_PATH ${CMAKE_SOURCE_DIR}/deps/dearimgui)
set(DEAR_IMGUI_BACKENDS_PATH ${DEAR_IMGUI_DEP_PATH}/backends)

set(EXE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/physics_game.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sdl_helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/texture.cpp)

add_executable(${EXE_NAME} ${DEAR_IMGUI_SOURCES} ${EXE_SOURCES})

target_compile_features(${EXE_NAME} PRIVATE cxx_std_20)
target_compile_definitions(${EXE_NAME} PRIVATE "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:${MAZE_DEBUG_DEF}>")

target_include_directories(${EXE_NAME} PRIVATE
    $<BUILD_INTERFACE:${DEAR_IMGUI_BACKENDS_PATH}>
    $<BUILD_INTERFACE:${DEAR_IMGUI_DEP_PATH}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/deps/stb>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/deps>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:deps/dearimgui>
    $<INSTALL_INTERFACE:deps/stb>)

target_link_libraries(${EXE_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_CORE_LIB}_static SDL3::SDL3 box2d::box2d)

if(EMSCRIPTEN)
    message(INFO ": Building ${EXE_NAME} for the Web!")

    set_target_properties(${EXE_NAME} PROPERTIES SUFFIX ".html" LINK_FLAGS "-lembind")

    set(EMSCRIPTEN_DEP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../deps/emscripten_local)
    target_include_directories(${EXE_NAME} PRIVATE $<INSTALL_INTERFACE:deps/emscripten_local> $<BUILD_INTERFACE:${EMSCRIPTEN_DEP_PATH}>)

    target_link_options(${EXE_NAME} PRIVATE
        "SHELL:-s WASM=1 -s MODULARIZE=1 -s EXPORT_ES6=1"
        "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/emscripten_local/shell_minimal.html"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/resources@/resources")

    target_link_options(${EXE_NAME} PRIVATE "$<$<CONFIG:Debug>:SHELL:-s STACK_OVERFLOW_CHECK=2 -s ASSERTIONS=2 -gsource-map --profile>")
    target_link_options(${EXE_NAME} PRIVATE "$<$<CONFIG:Release>:SHELL:-O3>")

else()
    message(INFO ": Building ${EXE_NAME} for the Desktop!")

    file(COPY "resources" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

	# Copy shared/dlls on Windows only
	if(WIN32)
		add_custom_command(
			TARGET ${EXE_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:${EXE_NAME}>
			VERBATIM
		)
	endif()

endif(EMSCRIPTEN)
