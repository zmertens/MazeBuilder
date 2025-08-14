set(GLAD_SOURCES ${CMAKE_SOURCE_DIR}/deps/glad/src/glad_3_0.c)
set(NOISE_SOURCES ${CMAKE_SOURCE_DIR}/deps/noise/noise.c)
set(SQLITE3_SOURCES ${CMAKE_SOURCE_DIR}/deps/sqlite/sqlite3.c)

set(CRAFT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
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

set(VOXELS_EXE "mazebuildervoxels")

if(EMSCRIPTEN)
    message(INFO ": Building ${VOXELS_EXE} examples for the Web!")

    add_executable(${VOXELS_EXE} ${GLAD_SOURCES} ${NOISE_SOURCES} ${SQLITE3_SOURCES} ${DEAR_IMGUI_SOURCES} ${CRAFT_SOURCES})
    set_target_properties(${VOXELS_EXE} PROPERTIES SUFFIX ".html" LINK_FLAGS "-lembind")

    target_link_libraries(${VOXELS_EXE} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_CORE_LIB}_static SDL3::SDL3)
    target_include_directories(${VOXELS_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/emscripten_local)
    target_compile_features(${VOXELS_EXE} PRIVATE cxx_std_17)
    target_include_directories(${VOXELS_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/glad/include)
    target_include_directories(${VOXELS_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps)
    target_include_directories(${VOXELS_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/dearimgui)
    target_include_directories(${VOXELS_EXE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
    
    target_link_options(${VOXELS_EXE} PRIVATE
    	"SHELL:-s PTHREAD_POOL_SIZE=5 -s WASM=1 -s USE_PTHREADS=1 -s ALLOW_MEMORY_GROWTH=1 -s STACK_SIZE=323232"
        "SHELL:-s FULL_ES3=1 -s MAX_WEBGL_VERSION=2 -s MODULARIZE=1 -s EXPORT_ES6=1 -s ASYNCIFY"
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/deps/emscripten_local/shell_minimal.html"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/shaders@/shaders"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/textures@/textures")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_options(${VOXELS_EXE} PRIVATE "SHELL:-s STACK_OVERFLOW_CHECK=2 -s SAFE_HEAP=2 -s ASSERTIONS=2 -gsource-map")
    else()
        target_link_options(${VOXELS_EXE} PRIVATE "SHELL:-O3")
    endif()
else()

    message(INFO ": Building ${VOXELS_EXE} examples for the Desktop!")

    add_executable(${VOXELS_EXE} ${GLAD_SOURCES} ${NOISE_SOURCES} ${SQLITE3_SOURCES} ${DEAR_IMGUI_SOURCES} ${CRAFT_SOURCES})
    target_link_libraries(${VOXELS_EXE} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_CORE_LIB}_static SDL3::SDL3)
    target_compile_features(${VOXELS_EXE} PRIVATE cxx_std_17)
    target_include_directories(${VOXELS_EXE} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
    target_include_directories(${VOXELS_EXE} PUBLIC $<INSTALL_INTERFACE:deps> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps>)
    target_include_directories(${VOXELS_EXE} PUBLIC $<INSTALL_INTERFACE:deps/dearimgui> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps/dearimgui>)
    target_include_directories(${VOXELS_EXE} PUBLIC $<INSTALL_INTERFACE:deps/glad/include> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps/glad/include>)
    target_compile_definitions(${VOXELS_EXE} PRIVATE "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:MAZE_DEBUG>")

    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_BINARY_DIR}/bin/Voxels")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/textures" DESTINATION "${CMAKE_BINARY_DIR}/bin/Voxels")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/secure_http_server.py" DESTINATION "${CMAKE_BINARY_DIR}/bin/Voxels")

    # Copy shared/dlls on Windows only
	if(WIN32)
		add_custom_command(
			TARGET ${VOXELS_EXE} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:${VOXELS_EXE}>
			VERBATIM
		)
	endif()

endif(EMSCRIPTEN)
