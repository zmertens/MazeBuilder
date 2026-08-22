# MazeBuilder HTTP example – server + SFML client in one executable
set(HTTP_EXE "mazebuilderhttp")

set(HTTP_SOURCES
    main.cpp
    maze_server.cpp)

message(STATUS "Http: building '${HTTP_EXE}' for desktop")

add_executable(${HTTP_EXE} ${HTTP_SOURCES})

target_link_libraries(${HTTP_EXE} PRIVATE
    ${MAZE_BUILDER_CORE_LIB}_static
    SFML::Network)

foreach(_sfml_network_dep libssh2_static mbedtls mbedcrypto mbedx509)
    if(TARGET ${_sfml_network_dep})
        add_dependencies(${HTTP_EXE} ${_sfml_network_dep})
    endif()
endforeach()

target_compile_features(${HTTP_EXE} PRIVATE cxx_std_17)

target_include_directories(${HTTP_EXE} PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_definitions(${HTTP_EXE} PRIVATE
    "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:${MAZE_DEBUG_DEF}>")

# Copy SFML shared libs next to the executable (only for shared builds)
if(WIN32 AND BUILD_SHARED_LIBS)
    foreach(_sfml_tgt SFML::Graphics SFML::Network)
        add_custom_command(
            TARGET ${HTTP_EXE} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
                $<TARGET_FILE:${_sfml_tgt}>
                $<TARGET_FILE_DIR:${HTTP_EXE}>
            VERBATIM)
    endforeach()
endif()
