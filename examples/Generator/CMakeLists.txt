
# Setup dependencies
set(GENERATOR_EXE "generator")

set(GENERATOR_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Generator.cpp)

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

include(FetchContent)
function(fetch_git_repo REPO_NAME REPO_URL REPO_TAG)
    if(NOT EXISTS ${REPO_DIR})
        message(INFO ": Fetching ${REPO_NAME} from ${REPO_URL}")
        FetchContent_Declare(${REPO_NAME} GIT_REPOSITORY ${REPO_URL} GIT_TAG ${REPO_TAG} GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
        FetchContent_MakeAvailable(${REPO_NAME})
    endif()
endfunction()

# Note that the latest SDL might not be available as an EMSDK port yet
if(MAZE_BUILD_WEB_EXAMPLES AND EMSCRIPTEN)

elseif(MAZE_BUILD_DESKTOP_EXAMPLES)
    message(INFO ": Building ${GENERATOR_EXE} for the Desktop!")
    
	# Handle SDL dependencies first
	find_package(SDL3 QUIET)
    if(NOT SDL3_FOUND)
        message(INFO ": Getting SDL via FetchContent")
        set(SDL_SHARED TRUE CACHE BOOL "Build a SDL shared library (if available)")
        set(SDL_STATIC FALSE CACHE BOOL "Build a SDL static library (if available)")
        fetch_git_repo(SDL https://github.com/libsdl-org/SDL.git preview-3.1.3)
        set_property(DIRECTORY "${CMAKE_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif()

    add_executable(${GENERATOR_EXE} ${GENERATOR_SRCS} ${DEAR_IMGUI_SOURCES})
    target_link_libraries(${GENERATOR_EXE} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_LIB}_static SDL3::SDL3)
    target_compile_features(${GENERATOR_EXE} PUBLIC cxx_std_17)
    target_include_directories(${GENERATOR_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
	target_include_directories(${GENERATOR_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps)
    target_include_directories(${GENERATOR_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/dearimgui)

    file(COPY "resources" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

	# Copy shared/dlls on Windows only
	if(WIN32)
		add_custom_command(
			TARGET ${GENERATOR_EXE} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:${GENERATOR_EXE}>
			VERBATIM
		)
	endif()

endif(MAZE_BUILD_WEB_EXAMPLES AND EMSCRIPTEN)
