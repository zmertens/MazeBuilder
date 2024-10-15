
# Setup dependencies
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

set(GLAD_SOURCES ${CMAKE_SOURCE_DIR}/deps/glad/src/glad_3_0.c)
set(NOISE_SOURCES ${CMAKE_SOURCE_DIR}/deps/noise/noise.c)
set(SQLITE3_SOURCES ${CMAKE_SOURCE_DIR}/deps/sqlite/sqlite3.c)

set(CRAFT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/craft.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cube.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/db.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/item.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/matrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ring.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sign.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/craft_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/world.cpp)

message(INFO ": Getting SDL via FetchContent")
include(FetchContent)
FetchContent_Declare(SDL GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG preview-3.1.3)
FetchContent_MakeAvailable(SDL)
set(SDL_SHARED FALSE CACHE BOOL "Build a SDL shared library (if available)")
set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")
set_property(DIRECTORY "${CMAKE_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)

set(VOXELS_EXE "voxels")
add_executable(${VOXELS_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ${DEAR_IMGUI_SOURCES} ${GLAD_SOURCES} ${NOISE_SOURCES} ${SQLITE3_SOURCES} ${CRAFT_SOURCES})
target_link_libraries(${VOXELS_EXE} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_LIB}_static SDL3::SDL3)
target_compile_features(${VOXELS_EXE} PUBLIC cxx_std_17)
target_include_directories(${VOXELS_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/deps/glad/include)
target_include_directories(${VOXELS_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/deps)
target_include_directories(${VOXELS_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/deps/emscripten_local)
target_include_directories(${VOXELS_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/deps/dearimgui)
target_include_directories(${VOXELS_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Note that the latest SDL might not be available as an EMSDK port yet
if(MAZE_BUILD_WEB_EXAMPLES AND EMSCRIPTEN)
    message(INFO ": Building SDL app for Web!")

    # Check if CMAKE_TOOLCHAIN_FILE is set, throw a fatal error if not
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE is not set. Please specify the path to the Emscripten toolchain file.")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -matomics -mbulk-memory")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -matomics -mbulk-memory")

    set_target_properties(${VOXELS_EXE} PROPERTIES SUFFIX ".html" LINK_FLAGS "-lembind")
    
    target_link_options(${VOXELS_EXE} PRIVATE
    	"SHELL:-s PTHREAD_POOL_SIZE=6 -s PTHREAD_POOL_SIZE_STRICT=2 -s WASM=1 -s USE_PTHREADS=1 -s ALLOW_MEMORY_GROWTH=1 -s STACK_SIZE=323232"
        "SHELL:-s FULL_ES3=1 -s MAX_WEBGL_VERSION=2 -s MODULARIZE=1 -s EXPORT_ES6=1 -s ASYNCIFY"
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/deps/emscripten_local/shell_minimal.html"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/shaders/es/block_vertex.es.glsl@/shaders/es/block_vertex.es.glsl"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/shaders/es/block_fragment.es.glsl@/shaders/es/block_fragment.es.glsl"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/shaders/es/line_vertex.es.glsl@/shaders/es/line_vertex.es.glsl"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/shaders/es/line_fragment.es.glsl@/shaders/es/line_fragment.es.glsl"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/shaders/es/sky_vertex.es.glsl@/shaders/es/sky_vertex.es.glsl"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/shaders/es/sky_fragment.es.glsl@/shaders/es/sky_fragment.es.glsl"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/shaders/es/text_vertex.es.glsl@/shaders/es/text_vertex.es.glsl"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/shaders/es/text_fragment.es.glsl@/shaders/es/text_fragment.es.glsl"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/textures/texture.png@/textures/texture.png"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/textures/font.png@/textures/font.png"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/textures/sign.png@/textures/sign.png"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/textures/sky.png@/textures/sky.png"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/textures/maze_in_green_32x32.bmp@/textures/maze_in_green_32x32.bmp")
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        target_link_options(${VOXELS_EXE} PRIVATE "SHELL:-s STACK_OVERFLOW_CHECK=2 -s SAFE_HEAP=2 -s ASSERTIONS=2 -gsource-map")
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_link_options(${VOXELS_EXE} PRIVATE "SHELL:-O3")
    endif()
endif(MAZE_BUILD_WEB_EXAMPLES AND EMSCRIPTEN)

file(COPY "${CMAKE_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_BINARY_DIR}/bin/Voxels")
file(COPY "${CMAKE_SOURCE_DIR}/textures" DESTINATION "${CMAKE_BINARY_DIR}/bin/Voxels")
file(COPY "${CMAKE_SOURCE_DIR}/scripts/secure_http_server.py" DESTINATION "${CMAKE_BINARY_DIR}/bin/Voxels")

# Copy over the freshly build SDL dll file to the binary directory
# if(WIN32)
#     add_custom_command(
#         TARGET ${VOXELS_EXE}
#         COMMENT "Copy SDL DLL"
#         POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/_deps/sdl-build/${CMAKE_BUILD_TYPE}/SDL3.dll $<TARGET_FILE_DIR:${VOXELS_EXE}>
#         VERBATIM)
# endif()
