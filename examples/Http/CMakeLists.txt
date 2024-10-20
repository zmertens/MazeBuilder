
# Set the name of the executable
set(MAZE_BUILDER_HTTP_EXE "http")
add_executable(${MAZE_BUILDER_HTTP_EXE} main.cpp)
target_link_libraries(${MAZE_BUILDER_HTTP_EXE} PRIVATE ${MAZE_BUILDER_LIB}_static)
target_include_directories(${MAZE_BUILDER_HTTP_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(${MAZE_BUILDER_HTTP_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/deps)
target_compile_features(${MAZE_BUILDER_HTTP_EXE} PUBLIC cxx_std_17)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(INFO ": ${CMAKE_BUILD_TYPE} - enabled ${PROJECT_NAME} to use -DMAZE_DEBUG flag.")
    target_compile_definitions(${MAZE_BUILDER_HTTP_EXE} PRIVATE "MAZE_DEBUG")
endif()

# Get SFML from the web
message(INFO ": Fetching SFML from https://github.com/SFML/SFML.git")
include(FetchContent)
FetchContent_Declare(SFML GIT_REPOSITORY https://github.com/SFML/SFML.git GIT_TAG 2.6.x GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
FetchContent_MakeAvailable(SFML)
target_link_libraries(${MAZE_BUILDER_HTTP_EXE} PRIVATE sfml-network)

