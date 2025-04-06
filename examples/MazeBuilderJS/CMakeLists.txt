
if(EMSCRIPTEN)
#    message(INFO ": Building ${PROJECT_NAME} for the Web!")

    # Check if CMAKE_TOOLCHAIN_FILE is set, throw a fatal error if not
#    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#        message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE is not set. Please specify the path to the Emscripten toolchain file.")
#    endif()

#    set(MAZE_BUILDER_JS "maze_builder_js")
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -matomics -mbulk-memory")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -matomics -mbulk-memory")
    
    # Set the output file
#    add_executable(${MAZE_BUILDER_JS} main.cpp)
    # add_executable(${MAZE_BUILDER_JS} main.cpp)
#    set_target_properties(${MAZE_BUILDER_JS} PROPERTIES SUFFIX ".html" LINK_FLAGS "-lembind")
#    target_link_options(${MAZE_BUILDER_JS} PRIVATE --emit-tsd "$<TARGET_FILE_DIR:${MAZE_BUILDER_JS}>/${MAZE_BUILDER_JS}.d.ts")
#    target_link_libraries(${MAZE_BUILDER_JS} ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_LIB}_static)
#    target_include_directories(${MAZE_BUILDER_JS} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    
#    target_link_options(${MAZE_BUILDER_JS} PRIVATE "SHELL: -s WASM=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s ASYNCIFY")
#    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
#        target_link_options(${MAZE_BUILDER_JS} PRIVATE "SHELL:-s STACK_OVERFLOW_CHECK=2 -s SAFE_HEAP=2 -s ASSERTIONS=2 -gsource-map")
#    endif()
#    if(CMAKE_BUILD_TYPE STREQUAL "Release")
#        target_link_options(${MAZE_BUILDER_JS} PRIVATE "SHELL:-O3")
#    endif()
endif(EMSCRIPTEN)

# Copy the example script to the build directory
file(COPY "${CMAKE_SOURCE_DIR}/scripts/run.js" DESTINATION "${CMAKE_BINARY_DIR}/bin/MazeBuilderJS")
file(COPY "${CMAKE_SOURCE_DIR}/scripts/package.json" DESTINATION "${CMAKE_BINARY_DIR}/bin/MazeBuilderJS")
