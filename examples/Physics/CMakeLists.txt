include(FetchGitRepo)

# Setup dependencies
set(EXE_NAME "mazebuilderphysics")

set(EXE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Ball.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CommandQueue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Entity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HttpClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Level.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Pathfinder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PhysicsGame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Player.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SceneNode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SDLHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SessionHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Sprite.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SpriteNode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Transformable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UdpClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Wall.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/WorkerConcurrent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/World.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/View.cpp)

if(EMSCRIPTEN)
    message(INFO ": Building ${EXE_NAME} examples for the Web!")

    add_executable(${EXE_NAME} ${DEAR_IMGUI_SOURCES} ${EXE_SOURCES})
    set_target_properties(${EXE_NAME} PROPERTIES SUFFIX ".html" LINK_FLAGS "-lembind")
    target_link_libraries(${EXE_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_CORE_LIB}_static SDL3::SDL3 SFML::Audio SFML::Network box2d::box2d)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/emscripten_local)
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_20)
    target_compile_options(${EXE_NAME} PRIVATE ${BASE_COMPILE_FLAGS} "-pthread")
    target_compile_definitions(${EXE_NAME} PRIVATE "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:${MAZE_DEBUG_DEF}>")
    target_include_directories(${EXE_NAME} PRIVATE $<INSTALL_INTERFACE:deps/glad/include> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps/glad/include>)
    target_include_directories(${EXE_NAME} PUBLIC $<INSTALL_INTERFACE:deps/dearimgui> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps/dearimgui>)
    target_include_directories(${EXE_NAME} PRIVATE $<INSTALL_INTERFACE:deps> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps>)
    target_include_directories(${EXE_NAME} PRIVATE $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

    target_link_options(${EXE_NAME} PRIVATE
        "SHELL:-s PTHREAD_POOL_SIZE=5 -s WASM=1 -s USE_PTHREADS -s ALLOW_MEMORY_GROWTH=1 -s STACK_SIZE=323232"
        "SHELL:-s FULL_ES3=1 -s MAX_WEBGL_VERSION=2 -s MODULARIZE=1 -s EXPORT_ES6=1"
        "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/emscripten_local/shell_minimal.html"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/resources@/resources")

    target_link_options(${EXE_NAME} PRIVATE "$<$<CONFIG:Debug>:SHELL:-s STACK_OVERFLOW_CHECK=2 -s ASSERTIONS=2 -gsource-map --profile>")
    target_link_options(${EXE_NAME} PRIVATE "$<$<CONFIG:Release>:SHELL:-O3>")

else()
    message(INFO ": Building ${EXE_NAME} for the Desktop!")

    add_executable(${EXE_NAME} ${EXE_SOURCES} ${DEAR_IMGUI_SOURCES})
    target_link_libraries(${EXE_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_CORE_LIB}_static SDL3::SDL3 box2d::box2d SFML::Audio SFML::Network)
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_20)
    target_compile_definitions(${EXE_NAME} PRIVATE "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:${MAZE_DEBUG_DEF}>")
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
        add_custom_command(
			TARGET ${EXE_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SFML::Network> $<TARGET_FILE_DIR:${EXE_NAME}>
			VERBATIM
		)
	endif()

endif(EMSCRIPTEN)
