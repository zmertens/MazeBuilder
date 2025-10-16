# Set the name of the executable
set(HTTP_EXE "mazebuilderhttp")

set(HTTP_SOURCES
    http_client.cpp
    main.cpp
    terminal_gui.cpp)

if(EMSCRIPTEN)

else()
    message(INFO ": Building ${HTTP_EXE} for the Desktop!")
    add_executable(${HTTP_EXE} ${HTTP_SOURCES})
    target_link_libraries(${HTTP_EXE} PRIVATE ${MAZE_BUILDER_CORE_LIB}_static SFML::Network)
    target_compile_features(${HTTP_EXE} PRIVATE cxx_std_17)
    target_include_directories(${HTTP_EXE} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
    target_include_directories(${HTTP_EXE} PUBLIC $<INSTALL_INTERFACE:deps> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/deps>)
    target_compile_definitions(${HTTP_EXE} PRIVATE "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:MAZE_DEBUG>")
	
	# Copy shared/dlls on Windows only
	if(WIN32)
		add_custom_command(
			TARGET ${HTTP_EXE} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SFML::Network> $<TARGET_FILE_DIR:${HTTP_EXE}>
			VERBATIM
		)
	endif()
endif()
