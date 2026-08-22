set(EXE_NAME "amazing")

set(EXE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/game/resource_paths.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/game/game_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/game/ui_menu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/game/animation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/game/player_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/game/collision_system.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/game/audio_system.cpp)

if(NOT EMSCRIPTEN)
    message(INFO " - Building desktop executable '${EXE_NAME}'")

    add_executable(${EXE_NAME} ${EXE_SOURCES})

    target_compile_features(${EXE_NAME} PRIVATE cxx_std_20)

    target_include_directories(${EXE_NAME} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

    target_link_libraries(${EXE_NAME} PRIVATE
        ${MAZE_BUILDER_CORE_LIB}_static
        box2d::box2d
        SFML::Graphics SFML::Network SFML::Audio)

    foreach(_sfml_graphics_dep SheenBidi harfbuzz freetype)
        if(TARGET ${_sfml_graphics_dep})
            add_dependencies(${EXE_NAME} ${_sfml_graphics_dep})
        endif()
    endforeach()

    target_compile_definitions(${EXE_NAME} PRIVATE
        "$<$<OR:$<STREQUAL:$<CONFIG>,Debug>,$<STREQUAL:$<CONFIG>,RelWithDebInfo>>:${MAZE_DEBUG_DEF}>")

    # Copy resources
    add_custom_command(TARGET ${EXE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/resource_paths.json"
            "$<TARGET_FILE_DIR:${EXE_NAME}>/resource_paths.json"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/launch.bat"
            "$<TARGET_FILE_DIR:${EXE_NAME}>/launch.bat"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/roguelike"
            "$<TARGET_FILE_DIR:${EXE_NAME}>/roguelike"
        VERBATIM)

    # On Windows, copy the SFML shared libraries next to the executable
    if(WIN32 AND BUILD_SHARED_LIBS)
        foreach(_sfml_tgt SFML::Graphics SFML::Network SFML::Audio)
            add_custom_command(
                TARGET ${EXE_NAME} POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy
                    $<TARGET_FILE:${_sfml_tgt}>
                    $<TARGET_FILE_DIR:${EXE_NAME}>
                VERBATIM)
        endforeach()
    endif()
endif()
