

# Set the name of the executable
set(EXE_NAME "mazebuildercli")
if(EMSCRIPTEN)

   message(INFO ": Building ${EXE_NAME} for the Web!")

   set(JS_FILE "mazebuilder")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -matomics -mbulk-memory")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -matomics -mbulk-memory")
    
   # Set the output file
   add_executable(${EXE_NAME} main.cpp cli.cpp)

   set_target_properties(${JS_FILE} PROPERTIES SUFFIX ".js" LINK_FLAGS "-lembind")
   target_link_options(${JS_FILE} PRIVATE --emit-tsd "$<TARGET_FILE_DIR:${JS_FILE}>/${JS_FILE}.d.ts")
   target_link_libraries(${JS_FILE} ${CMAKE_THREAD_LIBS_INIT} ${MAZE_BUILDER_CORE_LIB}_static)
   target_include_directories(${JS_FILE} PRIVATE ${CMAKE_SOURCE_DIR}/include)
   target_link_options(${JS_FILE} PRIVATE "SHELL: -s WASM=1 -s MODULARIZE=1 -s ASYNCIFY")

   if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_link_options(${JS_FILE} PRIVATE "SHELL:-O3")
   else()
        target_link_options(${JS_FILE} PRIVATE "SHELL:-s STACK_OVERFLOW_CHECK=2 -s ASSERTIONS=2")
        target_compile_definitions(${JS_FILE} PRIVATE MAZE_DEBUG)
   endif()

else()
    message(INFO ": Building ${EXE_NAME} for the Desktop!")
    add_executable(${EXE_NAME} main.cpp cli.cpp)
    target_link_libraries(${EXE_NAME} ${MAZE_BUILDER_CORE_LIB}_static)
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_17)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${EXE_NAME} PRIVATE MAZE_DEBUG)
   endif()
endif()
