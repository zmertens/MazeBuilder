# Set the name of the executable
set(EXE_NAME "mazebuildercli")

set(CLI_SOURCES
    cli.cpp
    main.cpp
    parser.cpp
)

if(EMSCRIPTEN)

    message(INFO ": Building ${EXE_NAME} for the Web!")
    
    # Set the output file
    add_executable(${EXE_NAME} ${CLI_SOURCES})
    set_target_properties(${EXE_NAME} PROPERTIES SUFFIX ".js" LINK_FLAGS "-lembind")
    target_compile_definitions(${EXE_NAME} PRIVATE "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:${MAZE_DEBUG_DEF}>")
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_20)
    target_compile_options(${EXE_NAME} PRIVATE ${BASE_COMPILE_FLAGS} "-pthread")
    target_link_options(${EXE_NAME} PRIVATE --emit-tsd "$<TARGET_FILE_DIR:${EXE_NAME}>/${EXE_NAME}.d.ts")
    target_link_libraries(${EXE_NAME} ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_CORE_LIB}_static)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_options(${EXE_NAME} PRIVATE "SHELL: -s WASM=1 -s MODULARIZE=1 -s ASYNCIFY")
    target_link_options(${EXE_NAME} PRIVATE "$<$<CONFIG:Debug>:SHELL:-s STACK_OVERFLOW_CHECK=2 -s ASSERTIONS=2 -gsource-map>")
    target_link_options(${EXE_NAME} PRIVATE "$<$<CONFIG:Release>:SHELL:-O3>")

else()

    message(INFO ": Building ${EXE_NAME} for the Desktop!")

    add_executable(${EXE_NAME} ${CLI_SOURCES})
    target_link_libraries(${EXE_NAME} ${MAZE_BUILDER_CORE_LIB}_static)
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_20)
    target_include_directories(${EXE_NAME} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
    target_compile_definitions(${EXE_NAME} PRIVATE "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:${MAZE_DEBUG_DEF}>")

endif()
