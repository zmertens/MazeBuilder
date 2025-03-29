

# Set the name of the executable
set(EXE_NAME "mazebuildercli")
if(EMSCRIPTEN)
else()
    message(INFO ": Building ${EXE_NAME} for the Desktop!")
    add_executable(${EXE_NAME} main.cpp)
    target_link_libraries(${EXE_NAME} ${MAZE_BUILDER_CORE_LIB}_static)
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_17)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(INFO ": Enabled ${EXE_NAME} to use -DMAZE_DEBUG flag.")
        target_compile_definitions(${EXE_NAME} PRIVATE "MAZE_DEBUG")
    endif()

endif()
