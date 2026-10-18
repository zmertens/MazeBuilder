set(EXE_NAME "amazing")

set(EXE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

message(INFO " - Building desktop executable '${EXE_NAME}'")

add_executable(${EXE_NAME} ${EXE_SOURCES})

target_compile_features(${EXE_NAME} PRIVATE cxx_std_20)

target_include_directories(${EXE_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(${EXE_NAME} PRIVATE
    ${MAZE_BUILDER_CORE_LIB}_static
    box2d::box2d
    SFML::Graphics)

foreach(_sfml_graphics_dep SheenBidi harfbuzz freetype)
    if(TARGET ${_sfml_graphics_dep})
        add_dependencies(${EXE_NAME} ${_sfml_graphics_dep})
    endif()
endforeach()

# Copy resources
add_custom_command(TARGET ${EXE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:${EXE_NAME}>/resources"
    VERBATIM)

# On Windows, copy the SFML shared libraries next to the executable
if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(
        TARGET ${EXE_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE_DIR:${EXE_NAME}>
        VERBATIM)
endif()

install(TARGETS ${EXE_NAME}
    RUNTIME DESTINATION bin/Amazing)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/
    DESTINATION bin/Amazing/resources)
