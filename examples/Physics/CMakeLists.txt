include(FetchGitRepo)

# Setup dependencies
set(EXE_NAME "mazebuilderphysics")

set(EXE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Ball.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Level.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OrthographicCamera.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Physics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Player.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SDLHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Wall.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/WorkerConcurrent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/World.cpp)

if(EMSCRIPTEN)
    message(INFO ": Building ${EXE_NAME} examples for the Web!")

    add_executable(${EXE_NAME} ${DEAR_IMGUI_SOURCES} ${EXE_SOURCES})
    set_target_properties(${EXE_NAME} PROPERTIES SUFFIX ".html" LINK_FLAGS "-lembind")

    target_link_libraries(${EXE_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_CORE_LIB}_static SDL3::SDL3 sfml-audio box2d::box2d)
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
        #target_link_options(${EXE_NAME} PRIVATE "$<$<CONFIG:Debug>:SHELL:-s STACK_OVERFLOW_CHECK=2 -s SAFE_HEAP=2 -s ASSERTIONS=2 -gsource-map>")
        target_link_options(${EXE_NAME} PRIVATE "SHELL:-s STACK_OVERFLOW_CHECK=2 -s SAFE_HEAP=2 -s ASSERTIONS=2 -gsource-map")
    else()
        #target_link_options(${EXE_NAME} PRIVATE "SHELL:-O3")
    endif()
else()
    message(INFO ": Building ${EXE_NAME} for the Desktop!")

    add_executable(${EXE_NAME} ${EXE_SOURCES} ${DEAR_IMGUI_SOURCES})
    target_link_libraries(${EXE_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_CORE_LIB}_static SDL3::SDL3 box2d::box2d SFML::Audio)
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_17)
    target_include_directories(${EXE_NAME} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
    target_include_directories(${EXE_NAME} PRIVATE $<INSTALL_INTERFACE:deps> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps>)
    target_include_directories(${EXE_NAME} PRIVATE $<INSTALL_INTERFACE:deps/dearimgui> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps/dearimgui>)

    file(COPY "resources" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

	# Copy shared/dlls on Windows only
	if(WIN32)
		add_custom_command(
			TARGET ${EXE_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:${EXE_NAME}>
			VERBATIM
		)
		add_custom_command(
			TARGET ${EXE_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SFML::Audio> $<TARGET_FILE_DIR:${EXE_NAME}>
			VERBATIM
		)
	endif()

endif(EMSCRIPTEN)
