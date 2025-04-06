
find_package(Catch2 3 QUIET)
if(CATCH2_FOUND)
    message(INFO ": Using Catch2 via find_package")
endif()
if(NOT CATCH2_FOUND)
    message(INFO ": Getting Catch2 via FetchContent")
    include(FetchGitRepo)
    FetchGitRepo(Catch2 https://github.com/catchorg/Catch2.git v3.8.0)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

enable_testing()
message(INFO ": Building ${PROJECT_NAME} tests")

file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)
set(MAZE_BUILDER_TESTS_EXE "mazebuildertests")
# These tests can use the Catch2-provided main
add_executable(${MAZE_BUILDER_TESTS_EXE} ${TEST_FILES})
add_test(NAME RUN_MAZE_TESTS COMMAND ${MAZE_BUILDER_TESTS_EXE})
target_compile_features(${MAZE_BUILDER_TESTS_EXE} PRIVATE cxx_std_17)
target_link_libraries(${MAZE_BUILDER_TESTS_EXE} PRIVATE ${MAZE_BUILDER_CORE_LIB}_static Catch2::Catch2WithMain)
target_include_directories(${MAZE_BUILDER_TESTS_EXE} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../deps")
target_include_directories(${MAZE_BUILDER_TESTS_EXE} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/maze_dfs.json" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if(MAZE_BUILDER_MEMCHECK)
	message(INFO ": Applying memcheck to target ${MAZE_BUILDER_TESTS_EXE}")
	include(Valgrind)
	AddValgrind(${MAZE_BUILDER_TESTS_EXE})
	include(Memcheck)
	AddMemcheck(${MAZE_BUILDER_TESTS_EXE})
endif()
