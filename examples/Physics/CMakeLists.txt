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
    message(INFO ": Building ${EXE_NAME} examples for the Web!")

    # Check if CMAKE_TOOLCHAIN_FILE is set, throw a fatal error if not
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE is not set. Please specify the path to the Emscripten toolchain file.")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -matomics -mbulk-memory")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -matomics -mbulk-memory")
    add_executable(${EXE_NAME} ${DEAR_IMGUI_SOURCES} ${EXE_SOURCES})
    set_target_properties(${EXE_NAME} PROPERTIES SUFFIX ".html" LINK_FLAGS "-lembind")

    set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")
    FetchGitRepo(SDL https://github.com/libsdl-org/SDL.git release-3.2.10)

    FetchGitRepo(box2d https://github.com/erincatto/box2d.git v3.1.0)
    set_property(TARGET box2d PROPERTY COMPILE_WARNING_AS_ERROR OFF)

    target_link_libraries(${EXE_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_CORE_LIB}_static SDL3::SDL3 box2d::box2d)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/emscripten_local)
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_17)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/glad/include)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/dearimgui)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

    target_link_options(${EXE_NAME} PRIVATE
        "SHELL:-s PTHREAD_POOL_SIZE=5 -s WASM=1 -s USE_PTHREADS=1 -s ALLOW_MEMORY_GROWTH=1 -s STACK_SIZE=323232"
        "SHELL:-s FULL_ES3=1 -s MAX_WEBGL_VERSION=2 -s MODULARIZE=1 -s EXPORT_ES6=1 -s ASYNCIFY"
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/deps/emscripten_local/shell_minimal.html"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/resources@/resources")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS ": Enabled ${EXE_NAME} to use -DMAZE_DEBUG flag.")
        target_link_options(${EXE_NAME} PRIVATE "SHELL:-s STACK_OVERFLOW_CHECK=2 -s SAFE_HEAP=2 -s ASSERTIONS=2 -gsource-map")
        target_compile_definitions(${EXE_NAME} PRIVATE "MAZE_DEBUG")
    else()
        target_link_options(${EXE_NAME} PRIVATE "SHELL:-O3")
    endif()
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

    # Add Box2D dependency
    find_package(box2d QUIET)
    if(NOT box2d_FOUND)
        message(INFO ": Getting Box2D via FetchContent")
        FetchGitRepo(box2d https://github.com/erincatto/box2d.git v3.1.0)
    endif()

    add_executable(${EXE_NAME} ${EXE_SOURCES} ${DEAR_IMGUI_SOURCES})
    target_link_libraries(${EXE_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_CORE_LIB}_static SDL3::SDL3 box2d::box2d)
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_17)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
	target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/dearimgui)

    # Add Box2D include directory if not already included by find_package
    if(TARGET box2d)
        set_property(TARGET box2d PROPERTY COMPILE_WARNING_AS_ERROR OFF)
        get_target_property(BOX2D_INCLUDE_DIRS box2d INTERFACE_INCLUDE_DIRECTORIES)
        if(BOX2D_INCLUDE_DIRS)
            target_include_directories(${EXE_NAME} PRIVATE ${BOX2D_INCLUDE_DIRS})
        endif()
    endif()

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
