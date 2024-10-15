
option(MAZE_BUILD_TESTS "Build ${PROJECT_NAME} tests" OFF)

if(MAZE_BUILD_TESTS)
    find_package(Catch2 3 QUIET)
    if(CATCH2_FOUND)
        message(INFO ": Getting Catch2 via find_package")
    endif()
    if(NOT CATCH2_FOUND)
        message(INFO ": Getting Catch2 via FetchContent")
        include(FetchContent)
        FetchContent_Declare(Catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git GIT_TAG v3.7.0)
        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    endif()

    enable_testing()
    message(INFO ": Building ${PROJECT_NAME} tests")

    file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp)
    set(MAZE_BUILDER_TESTS_EXE "maze_builder_tests")
    # These tests can use the Catch2-provided main
    add_executable(${MAZE_BUILDER_TESTS_EXE} ${TEST_FILES})
    add_test(NAME RUN_MAZE_TESTS COMMAND ${MAZE_BUILDER_TESTS_EXE})
    target_compile_features(${MAZE_BUILDER_TESTS_EXE} PUBLIC cxx_std_17)
    target_link_libraries(${MAZE_BUILDER_TESTS_EXE} PRIVATE ${MAZE_BUILDER_LIB} Catch2::Catch2WithMain)
    target_include_directories(${MAZE_BUILDER_TESTS_EXE} PRIVATE "${CMAKE_SOURCE_DIR}/deps")
endif()
