
# Set the name of the executable
set(MAZE_BUILDER_EXE "maze_builder_network")
add_executable(${MAZE_BUILDER_EXE} main.cpp)
target_link_libraries(${MAZE_BUILDER_EXE} PRIVATE ${MAZE_BUILDER_LIB}_static)
target_include_directories(${MAZE_BUILDER_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_features(${MAZE_BUILDER_EXE} PUBLIC cxx_std_17)

# Get SFML from the web
message(INFO ": Fetching SFML from https://github.com/SFML/SFML.git")
include(FetchContent)
FetchContent_Declare(SFML GIT_REPOSITORY https://github.com/SFML/SFML.git GIT_TAG 2.6.x GIT_SHALLOW TRUE GIT_PROGRESS TRUE)
FetchContent_MakeAvailable(SFML)
target_link_libraries(${MAZE_BUILDER_EXE} PRIVATE sfml-network)

