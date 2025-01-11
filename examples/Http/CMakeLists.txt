# Set the name of the executable
set(HTTP_EXE "mazebuilderhttp")
add_executable(${HTTP_EXE} main.cpp)
target_link_libraries(${HTTP_EXE} PRIVATE ${MAZE_BUILDER_CORE_LIB}_static)
target_include_directories(${HTTP_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(${HTTP_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/deps)
target_compile_features(${HTTP_EXE} PRIVATE cxx_std_17)

# Get SFML from the web
find_package(SFML 3.0.0 COMPONENTS network)
if(NOT SFML_FOUND)
    include(FetchGitRepo)
    set(SFML_REPO_URL "https://github.com/SFML/SFML.git")
    message(INFO ": Fetching SFML from ${SFML_REPO_URL}")
    FetchGitRepo(SFML ${SFML_REPO_URL} 3.0.0)
endif()

target_link_libraries(${HTTP_EXE} PRIVATE sfml-network)

