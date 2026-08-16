set(EXE_NAME "amazingsfml")

set(EXE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

if(NOT EMSCRIPTEN)
    message(STATUS "AmazingSFML: building desktop executable '${EXE_NAME}'")

    add_executable(${EXE_NAME} ${EXE_SOURCES})

    target_compile_features(${EXE_NAME} PRIVATE cxx_std_20)

    target_include_directories(${EXE_NAME} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

    target_link_libraries(${EXE_NAME} PRIVATE
        ${MAZE_BUILDER_CORE_LIB}_static
        box2d::box2d
        SFML::Graphics
        SFML::Network)

    target_compile_definitions(${EXE_NAME} PRIVATE
        "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:${MAZE_DEBUG_DEF}>")

    # Copy resources
    add_custom_command(TARGET ${EXE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/../Voxels/textures/icon.bmp"
            "$<TARGET_FILE_DIR:${EXE_NAME}>/icon.bmp"
        VERBATIM)

    # On Windows, copy the SFML shared libraries next to the executable
    if(WIN32 AND BUILD_SHARED_LIBS)
        foreach(_sfml_tgt SFML::Graphics SFML::Network)
            add_custom_command(
                TARGET ${EXE_NAME} POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy
                    $<TARGET_FILE:${_sfml_tgt}>
                    $<TARGET_FILE_DIR:${EXE_NAME}>
                VERBATIM)
        endforeach()
    endif()
endif()
