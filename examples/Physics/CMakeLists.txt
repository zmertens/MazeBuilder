include(FetchGitRepo)

# Setup dependencies
set(EXE_NAME "mazebuilderphysics")

set(EXE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Physics.cpp)

set(DEAR_IMGUI_SOURCES
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui.cpp
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui_demo.cpp
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui_draw.cpp
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui_tables.cpp
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui_widgets.cpp
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_sdl3.cpp
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_opengl3.cpp)

set(DEAR_IMGUI_HEADERS
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui.h
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/imconfig.h
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/imgui_internal.h
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/imstb_rectpack.h
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/imstb_textedit.h
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/imstb_truetype.h
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_sdl3.h
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_opengl3.h
  ${CMAKE_SOURCE_DIR}/deps/dearimgui/backends/imgui_impl_opengl3_loader.h)

if(EMSCRIPTEN)

else()
    message(INFO ": Building ${EXE_NAME} for the Desktop!")
    
	# Handle SDL dependencies first
	find_package(SDL3 QUIET)
    if(NOT SDL3_FOUND)
        message(INFO ": Getting SDL via FetchContent")
        set(SDL_SHARED TRUE CACHE BOOL "Build a SDL shared library (if available)")
        set(SDL_STATIC FALSE CACHE BOOL "Build a SDL static library (if available)")
        FetchGitRepo(SDL https://github.com/libsdl-org/SDL.git release-3.2.10)
        set_property(DIRECTORY "${CMAKE_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif()

    add_executable(${EXE_NAME} ${EXE_SOURCES} ${DEAR_IMGUI_SOURCES})
    target_link_libraries(${EXE_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_CORE_LIB}_static SDL3::SDL3)
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_17)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
	target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/dearimgui)

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
